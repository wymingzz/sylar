#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar{

// 单例模式的模板类
template<class T, class x = void, int N = 0>
class Singleton{
public:
static T* GetInstance(){
    static T v;
    return &v;
}
};

template<class T, class x = void, int N = 0>
class SingletonPtr{
    public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}

#endif