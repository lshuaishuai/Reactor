#include "Buffer.h"

Buffer::Buffer(uint16_t sep)
    :sep_(sep)
{}

Buffer::~Buffer() {}
// 把数据追加到buf_中
void Buffer::append(const char* data, size_t size) { buf_.append(data, size); }

// 把数据追加到buffer中，附加报文头部
void Buffer::appendwithsep(const char* data, size_t size)
{
    if(sep_ == 0) buf_.append(data, size);           // 处理报文内容
    else if(sep_ == 1)
    {
        buf_.append((char*)&size, 4);      // 处理报文长度
        buf_.append(data, size);           // 处理报文内容
    }
    else if(sep_ == 2)
    {
        // 自己实现
    }
}

// 返回buf_的大小
size_t Buffer::size() { return buf_.size(); }                                      

// 返回buf_的首地址
const char* Buffer::data() { return buf_.data(); }        

// 清空buf_
void Buffer::clear() { buf_.clear(); }  

// 删除pos位置后的nn个字节的数据
void Buffer::erase(size_t pos, size_t nn) { buf_.erase(pos, nn); }                  

// 从buf_中拆分出一个报文，存放在ss_中，如果buf_中没有报文，返回false
bool Buffer::pickmessage(std::string& ss)
{
    if(buf_.size() == 0) return false;
    if(sep_ == 0)         // 固定长度 
    {
        ss = buf_; 
        buf_.clear();
    }
    else if(sep_ == 1)    // 四字节报头
    {
        // 下面这段代码可以封装在Buffer类中 指定报文长度
        int len;
        memcpy(&len, buf_.data(), 4);     // 从inputbuffer_中获取报文头部
        // 如果inputbuffer_中的数据量小于len+4字节，说明inputbuffer中的报文内容不完整，需要继续读取
        if(buf_.size() < len+4) return false;

        ss = buf_.substr(4, len); // 从第五个字节开始读取len个字节，因为前四个字节为报文长度len
        buf_.erase(0, len+4);                    // 将截取出来的报文从输入缓冲区中删除
    }
    else if(sep_ == 2)    // 分隔符
    {

    }
    return true;
}                  