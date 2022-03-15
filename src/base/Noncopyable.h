#ifndef WFREST_NONCOPYABLE_H_
#define WFREST_NONCOPYABLE_H_

namespace wfrest
{

class Noncopyable
{
public:
    Noncopyable(const Noncopyable&) = delete;
    void operator=(const Noncopyable&) = delete;

protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

} // namespace wfrest

#endif  // WFREST_NONCOPYABLE_H_