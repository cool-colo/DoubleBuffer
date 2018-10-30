#include <stdio.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <algorithm>

template<class T>
class DataWrapper{
public:
  DataWrapper(const T& data, std::shared_ptr<std::mutex>&& mutex)
    :data_(data), mutex_(std::move(mutex)){
    mutex_->lock();
  }
  ~DataWrapper(){
    mutex_->unlock();
  }
  const T& value()
  {
    return data_;
  }
  const T& data_;
  std::shared_ptr<std::mutex> mutex_;
}; 

template<typename T>
class DoubleBuffer{
    
public:
  template<class ... Args>
  DoubleBuffer(Args ... args){
    std::get<0>(data_) = T(std::forward<Args>(args)...);
    std::get<1>(data_) = T(std::forward<Args>(args)...);
  }   
  auto Get(){
    auto mutex = GetTLSMutexPtr();
    return std::make_shared<DataWrapper<T>>(UnsafeRead(), std::move(mutex));
  } 
    
  template<typename Fn>
  void Write(Fn&& fn){
    std::lock_guard<std::mutex> write_lock(write_mutex_);
    auto background_index = !index_.load(std::memory_order_relaxed);
    fn(data_[background_index]);
    index_.store(background_index, std::memory_order_release);
    
    {
      std::lock_guard<std::mutex> lock(mutex_);
      std::for_each(std::begin(read_mutex_), std::end(read_mutex_), [](std::shared_ptr<std::mutex>& read_mutex){
        std::lock_guard<std::mutex> lock(*read_mutex);
      });
    }
    
    fn(data_[!background_index]);
    
  } 
private:
  std::shared_ptr<std::mutex> GetTLSMutexPtr(){
    static thread_local std::shared_ptr<std::mutex> read_mutex = [this]{
      std::lock_guard<std::mutex> lock(mutex_);
      read_mutex_.push_back(std::make_shared<std::mutex>());
      return read_mutex_.back();
    }();
    return read_mutex;
  }
  const T& UnsafeRead(){
    return data_[index_.load(std::memory_order_acquire)];
  }
    
private:
  std::mutex write_mutex_;
  std::mutex mutex_;
  std::vector<std::shared_ptr<std::mutex>> read_mutex_;
  std::atomic<int> index_{0};
  std::array<T,2> data_;
};

int main()
{
    printf("Hello World22222");
    DoubleBuffer<int> db;
    return 0;
}
