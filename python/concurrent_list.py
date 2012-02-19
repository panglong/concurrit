#/**
# * Copyright (c) 2010-2011,
# * Tayfun Elmas    <elmas@cs.berkeley.edu>
# * All rights reserved.
# * <p/>
# * Redistribution and use in source and binary forms, with or without
# * modification, are permitted provided that the following conditions are
# * met:
# * <p/>
# * 1. Redistributions of source code must retain the above copyright
# * notice, this list of conditions and the following disclaimer.
# * <p/>
# * 2. Redistributions in binary form must reproduce the above copyright
# * notice, this list of conditions and the following disclaimer in the
# * documentation and/or other materials provided with the distribution.
# * <p/>
# * 3. The names of the contributors may not be used to endorse or promote
# * products derived from this software without specific prior written
# * permission.
# * <p/>
# * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# */
 
from counit import coroutine

class concurrent_list:

    class node:
        def __init__(self, elt, next = None):
            self.elt = elt
            self.next = next
            self.marked = False

    def __init__(self, *args):
        self.head = concurrent_list.node(None, None)
        self.tail = self.head
        self.capacity = 0
        for n in args:
            self.add(n)

    def __eq__(self, other):
        this_list = self.contents()
        if isinstance(other, list):
            that_list = other
        elif isinstance(other, concurrent_list):
            that_list = other.contents()
        else:
            return NotImplemented
        return this_list == that_list

    def __ne__(self, other):
        result = self.__eq__(other)
        if result is NotImplemented:
            return result
        return not result

    def __str__(self):
        return self.contents().__str__()

    def __contains__(self, item):
        return item in self.contents()

    def size(self):
        sz = 0
        node = self.head.next
        while node != None:
            sz += 1
            node = node.next
        return sz

    def is_empty(self):
        sz = self.size()
        return (sz == 0)

    def add(self, elt, sc = []):
        # yield
        coroutine.transfer(label="add_1")

        if self.capacity > 0 and self.size() == self.capacity:
            raise RuntimeError("Capacity is reached!")

        node = concurrent_list.node(elt, None)
        self.tail.next = node
        self.tail = node

        # yield
        coroutine.transfer(label="add_2")

    # take blocks
    def take(self):
        # block until the list is non-empty
        while self.is_empty():
            # yield
            coroutine.transfer(label="take_1")
            pass

        assert self.head != self.tail
        assert self.head.next != None
        elt = self.head.next.elt
        self.head = self.head.next
        # yield
        coroutine.transfer(label="take_2")
        return elt

    def remove(self, elt):
        assert elt != None
        node = self.head
        nn = node.next
        while nn != None:
            e = nn.elt
            if e == elt:
                nn.marked = True
                node.next = nn.next
                if nn == self.tail:
                    self.tail = node

                # yield
                coroutine.transfer(label="remove_1")
                return True

            node = nn
            nn = node.next

        #yield
        coroutine.transfer(label="remove_2")
        return False

    def remove_all(self, elt):
        num_removed = 0
        assert elt != None
        node = self.head
        nn = node.next
        while nn != None:
            e = nn.elt
            if e == elt:
                nn.marked = True
                node.next = nn.next
                num_removed += 1
                if nn == self.tail:
                    self.tail = node
                    break

                nn = nn.next # iterate nn
                # yield
                coroutine.transfer(label="remove_all_1")
                continue

            node = nn
            nn = node.next

        #yield
        coroutine.transfer(label="remove_all_2")
        return num_removed


    def contents(self):
        l = []
        node = self.head.next
        while node != None:
            if not node.marked:
                l.append(node.elt)
            node = node.next
        return l

    def foreach(self, f):
        node = self.head.next
        while node != None:
            if not node.marked:
                elt = node.elt
                f(elt)
                #yield
                coroutine.transfer(label="foreach")
            node = node.next
  