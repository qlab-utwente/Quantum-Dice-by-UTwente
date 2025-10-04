#ifndef ARRAY_H_
#define ARRAY_H_

template <typename T>
class Queue
{
private:
  T* data;
  size_t count;
  size_t capacity;
  size_t head;
  size_t tail;
private:
  void resize(size_t newCapacity)
  {
    T* newData = new T[newCapacity];
    for (size_t i = 0; i <= count; i++) {
      newData[i] = data[(head + i) % capacity];
    }

    delete[] data;
    data = newData;
    capacity = newCapacity;
    head = 0;
    tail = count;
  }

public:
  Queue(size_t initial_capacity)
  {
    count = 0;
    head = 0;
    tail = 0;
    capacity = initial_capacity;
    data = new T[capacity];
  }

  Queue()
  {
    count = 0;
    head = 0;
    tail = 0;
    capacity = 2;
    data = new T[capacity];
  }

  ~Queue()
  {
    delete[] data;
    data = 0;
  }

  void push(T item)
  {
    if (count == capacity) {
      resize(capacity * 2);
    }

    data[tail] = item;
    tail = (tail + 1) % capacity;
    count++;
  }

  T pop()
  {
    assert(count > 0);
    T item = data[head];
    head = (head + 1) % capacity;
    count--;
    return item;
  }

  bool isEmpty() const { return count == 0; }
  size_t size() const { return count; }
};

#endif
