#ifndef ATP_COMMON_MOVING_WINDOW_H_
#define ATP_COMMON_MOVING_WINDOW_H_

#include <string>
#include <vector>

#include <boost/ref.hpp>

#include "common/moving_window_callback.hpp"
#include "common/moving_window_interval_policy.hpp"


using boost::function;
using std::pair;
using std::string;
using std::vector;


namespace atp {
namespace time_series {

using namespace callback;

/// A moving window of time-based values
/// Different strategies for sampling can be set up.
template <
  typename element_t,
  typename sampler_t = function<element_t(const element_t& last,
                                          const element_t& current,
                                          bool new_sample_period) >,
  typename PostProcess = moving_window_post_process<microsecond_t, element_t>,
  typename Alloc = boost::pool_allocator<element_t>,
  typename time_interval_policy = time_interval_policy::align_at_zero >
class moving_window : public data_series<microsecond_t, element_t>
{
 public:

  typedef
  function< element_t(const data_series<microsecond_t, element_t>&) >
  series_operation;

  typedef
  function< element_t(const microsecond_t*, const element_t*, const size_t) >
  array_operation;


  /// total duration, time resolution, and initial value
  moving_window(boost::posix_time::time_duration h, sample_interval_t i,
                element_t init) :
      history_duration_(h),
      interval_(i),
      buffer_(h.total_microseconds() / i.total_microseconds()),
      init_(init),
      current_value_(init),
      current_ts_(0),
      collected_(0),
      time_interval_policy_(i.total_microseconds())
  {
    // fill the buffer with the default valule.
    for (size_t i = 0; i < buffer_.capacity(); ++i) {
      buffer_.push_back(init);
    }
  }

  /// Returns the duration of the history maintained by this moving_window
  const time_duration& history_duration() const
  {
    return history_duration_;
  }

  size_t total_events() const
  {
    return collected_;
  }

  size_t capacity() const
  {
    return buffer_.capacity() + 1;
  }

  /// Support for negative indexing as python
  /// -1 means the current value, -2 last element in buffer, ...
  virtual element_t operator[](int index) const
  {
    if (index == 0) {
      return current_value_;
    } else if (index < 0) {
      return buffer_[buffer_.size() + index];
    }
    return boost::lexical_cast<element_t>(init_);
  }

  /// Returns the time period of samples, eg. 1 usec or 1 min
  virtual sample_interval_t time_period() const
  {
    return interval_;
  }

  virtual size_t size() const
  {
    return buffer_.size() + 1; // plus current unpushed value.
  }

  /// Must have negative index
  /// 0 means current observation for the current time interval
  /// -1 means the previous interval
  virtual microsecond_t get_time(int offset = 0) const
  {
    return time_interval_policy_.get_time(current_ts_, -offset);
  }

  template <typename buffer_t>
  size_t copy_last(microsecond_t *timestamp,
                   buffer_t *array,
                   const size_t length) const
  {
    if (length > capacity()) {
      return 0; // No copy is done.
    }
    array[length - 1] = current_value_;
    timestamp[length - 1] = time_interval_policy_.get_time(current_ts_, 0);
    size_t to_copy = length - 1;
    size_t copied = 1;
    reverse_itr r = buffer_.rbegin();

    // fill the array in reverse order
    for (; r != buffer_.rend() && to_copy > 0; --to_copy, ++copied, ++r) {
      array[length - 1 - copied] = *r;

      timestamp[length - 1 - copied] =
          time_interval_policy_.get_time(current_ts_, copied);
    }
    return copied;
  }

  /// Returns the number of pushes into the time slots
  /// This allows the caller to track the number aggregated samples.
  size_t on(const microsecond_t& timestamp, const element_t& value)
  {
    return (*this)(timestamp, value);
  }

  size_t operator()(const microsecond_t& timestamp, const element_t& value)
  {
    int windows = time_interval_policy_.count_windows(current_ts_, timestamp);
    // Don't fill in missing values if starting up.
    if (current_ts_ > 0) {
      for (int i = 0; i < windows; ++i) {
        buffer_.push_back(current_value_);
        collected_++;
      }
    } else {
      windows = 0;
    }
    current_value_ = sampler_(current_value_, value,
                              windows > 0 || current_ts_ == 0);
    current_ts_ = timestamp;

    // TODO - compute dependent indicators

    // Call the post process callback after time have advanced to the
    // next window
    size_t p = static_cast<size_t>(windows);
    if (p > 0) post_process_(p, *this);

    return p;
  }

  void apply(const string& id, series_operation op,
             const size_t min_samples = 1)
  {
    series_operations.push_back(series_operation_pair(id, op));
  }


 private:

  typedef typename boost::circular_buffer<element_t, Alloc> history_t;
  typedef typename history_t::const_reverse_iterator reverse_itr;

  time_duration history_duration_;
  sample_interval_t interval_;
  history_t buffer_;

  element_t init_;
  element_t current_value_;
  microsecond_t current_ts_;
  size_t collected_;

  sampler_t sampler_;
  time_interval_policy time_interval_policy_;

  PostProcess post_process_;

  typedef pair<string, series_operation> series_operation_pair;
  vector<series_operation_pair> series_operations;

};



} // time_series
} // atp


#endif //ATP_COMMON_MOVING_WINDOW_H_
