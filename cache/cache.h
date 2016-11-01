/*******************************
FileName        :cache.h
Version         :
Date of Creation:2016/11/01
Description     :
Author          :codixu
*******************************/

#ifndef __H_CACHE_H_
#define __H_CACHE_H_

#include <sys/types.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <list>

using namespace std;

#define CACHE_LOG(fmt, args...) printf("%s:%d(%s):" fmt, __FILE__, __LINE__, __FUNCTION__, ## args);

template <typename TNodeType>
class CCache
{
private:
    enum UseFlag
    {
        UF_UNUSE = 0,
        UF_USE = 1,
    };
    
    //Cache头部信息，存放cache大小，使用情况等信息
    typedef struct _CacheHeader 
    {
        uint64_t size;
        uint64_t total_num;
        uint64_t free_num;
        uint64_t last_idx;

        _CacheHeader()
            :size(0),
            total_num(0),
            free_num(0),
            last_idx(0)
        {
            
            
        }

        string to_str()
        {
            stringstream ss;
            ss << "size:" << size
                << ", total_num:" << total_num
                << ", free_num:" << free_num
                << ", last_idx:" << last_idx;
            return ss.str();   
        }
        
    }CacheHeader;

    //Index，建立使用情况的映射关系
    typedef struct _CacheIdx
    {
        uint8_t  use_flag;
    }CacheIdx;

    typedef std::list<TNodeType*>  TNodeList;
    
private:
    char* m_pbase;
    CacheHeader* m_phead;
    CacheIdx* m_pidx;
    TNodeType* m_pcache;

public:   
    CCache()
    {
        m_pbase = NULL;
        m_phead = NULL;
        m_pidx = NULL;
        m_pcache = NULL;
    }
    
    int init(const uint64_t num, char* pbuf)
    {
        if (pbuf == NULL)
        {
            return -1;
        }

        if (num == 0)
        {
            return -2;
        }

        m_pbase = pbuf;
        m_phead = (CacheHeader*) m_pbase;
        
        //未初始化
        if (m_phead->total_num == 0)
        {
            memset(m_pbase, 0, get_size(num));
            m_phead->total_num = num;
            m_phead->free_num = num;
            m_phead->last_idx = 0;
            m_phead->size = get_size(num);
        }
        else
        {
        
        }

        //移到这里为了得到正确的total_num计算指针偏移量
        m_pidx = (CacheIdx*) (m_pbase + sizeof(CacheHeader));
        m_pcache =  (TNodeType*) (m_pbase + sizeof(CacheHeader) + sizeof(CacheIdx) * (m_phead->total_num + 1));
         
        return 0;
    }

    int ins(const TNodeType& lsh, uint64_t& id)
    {
        id = generate_idx();
        if (id == 0 || id > m_phead->total_num)
        {
            return -1;
        }

        m_pidx[id].use_flag = UF_USE;
        memcpy(&m_pcache[id], &lsh, sizeof(TNodeType));
        m_phead->free_num--;
            
        return 0;
    }

    TNodeType* get(uint64_t id)
    {
        //超出最大值
        if (id > m_phead->total_num)
        {
            return NULL;
        }

        //未使用
        if (m_pidx[id].use_flag != UF_USE)
        {
            return NULL;
        }

        //正常返回
        return &m_pcache[id];
    }

    int val(TNodeList& node_list)
    {
        node_list.clear();
        
        TNodeType* p_node = NULL;
        for(uint64_t i = 1; i <= m_phead->total_num; i++)
        {
            if (m_pidx[i].use_flag == UF_USE)
            {
                p_node = (TNodeType*)&m_pcache[i];
                node_list.push_back(p_node);
            }
        }

        return 0;
    }

    int val(std::list<uint64_t>& id_list)
    {
        id_list.clear();

        for(uint64_t i = 1; i <= m_phead->total_num; i++)
        {
            if (m_pidx[i].use_flag == UF_USE)
            {
                id_list.push_back(i);
            }
        }

        return 0;
    }
    
    int del(uint64_t id)
    {
        if (id > m_phead->total_num)
        {
            return -1;
        }

        if (m_pidx[id].use_flag == UF_USE)
        {
            m_pidx[id].use_flag = UF_UNUSE;
            m_phead->free_num++;
            //memset(&m_pcache[id], 0, sizeof(TNodeType));
        }

        return 0;
    }

    size_t get_size(uint64_t num)
    {
        return sizeof(CacheHeader) + sizeof(CacheIdx) * (num + 1) + sizeof(TNodeType) * (num + 1);
    }

    uint64_t generate_idx()
    {
        //CACHE_LOG("head info:%s\n", m_phead->to_str().c_str());
        
        if (m_phead->free_num == 0)
        {
            CACHE_LOG("free id use over. head info:%s\n", m_phead->to_str().c_str());
            return 0;
        }
        
        if (m_phead->last_idx < m_phead->total_num)
        {
            m_phead->last_idx++;
        }
        
        uint64_t id = m_phead->last_idx;
        
        //查看最后一次的free_id
        if (m_pidx[id].use_flag != UF_USE)
        {
            return id;
        }
        

        //需要重新扫描
        for(uint64_t i = id; i <= m_phead->total_num; ++i)
        {
            if(m_pidx[i].use_flag != UF_USE)
            {
                id = i;
                m_phead->last_idx = i;
                return id;
            }
        }

        //CACHE_LOG("can not find free id in last part. head info:%s\n", m_phead->to_str().c_str());
        
        //如果仍然没有找到，扫面前半部分
        for(uint64_t i = 1; i < m_phead->last_idx; ++i)
        {
            if(m_pidx[i].use_flag != UF_USE)
            {
                id = i;
                m_phead->last_idx = i;
                return id;
            }
        }

        //cache已经被用完
        //CACHE_LOG("free id is not in the first part. head info:%s\n", m_phead->to_str().c_str());
        
        return 0;
    }

    string to_str()
    {
        string str_out = m_phead->to_str();

        stringstream ss;
        for(size_t i = 1; i <= m_phead->total_num; i++)
        {
            ss << int(m_pidx[i].use_flag) << "|";
            if (i % 8 == 0)
            {
                ss << "\n";
            }
        }
        
        str_out += '\n';
        str_out += ss.str();
        return str_out;
    }
};

#endif

