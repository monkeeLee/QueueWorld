/*
    Queue World is copyright (c) 2014 Ross Bencina

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef INCLUDED_QWMPSCFIFOQUEUE_H
#define INCLUDED_QWMPSCFIFOQUEUE_H

#include "QwSingleLinkNodeInfo.h"
#include "QwMPMCPopAllLifoStack.h"
#include "QwSTailList.h"


// A multiple producer, single consumer queue. Can safely be posted to from
// multiple threads at once.
// This uses the "Reversed IBM Freelist" technique

template<typename NodePtrT, int NEXT_LINK_INDEX>
class QwMPSCFifoQueue {

    typedef QwSingleLinkNodeInfo<NodePtrT,NEXT_LINK_INDEX> nodeinfo;

    QwMPMCPopAllLifoStack<NodePtrT, NEXT_LINK_INDEX> mpscLifo_;
    QwSTailList<NodePtrT, NEXT_LINK_INDEX> consumerLocalReversingQueue_;

public:
    typedef typename nodeinfo::node_type node_type;
    typedef typename nodeinfo::node_ptr_type node_ptr_type;
    typedef typename nodeinfo::const_node_ptr_type const_node_ptr_type;

    void send( node_ptr_type n, bool& wasEmpty )
    {
        return mpscLifo_.push(n, wasEmpty);
    }

    node_ptr_type receive()
    {
        if (consumerLocalReversingQueue_.empty()) {
            if (mpscLifo_.empty()) {
                return 0;
            } else {
                node_ptr_type n = mpscLifo_.pop_all();
                if (n) {
                    // push all but the last node popped from the lifo into consumerLocalReversingQueue_
                    // this reverses their order, putting them into fifo order.
                    node_ptr_type next = nodeinfo::next_ptr(n);
                    while (next != 0) {
                        nodeinfo::clear_node_link_for_validation(n);
                        consumerLocalReversingQueue_.push_front(n);
                        n = next;
                        next = nodeinfo::next_ptr(n);
                    }
                    // return the last request, which is the next in fifo order
                    return n; // n->next is always zero here.
                } else {
                    return 0;
                }
            }
            
        } else {
            // (know consumerLocalReversingQueue_ is not empty here)
            return consumerLocalReversingQueue_.pop_front();
        }
    }
};

#endif /* INCLUDED_QWMPSCFIFOQUEUE_H */