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

import counit

__author__ = 'tayfun elmas'

import unittest
from concurrent_list import concurrent_list
from counit import coroutine, Assume, Assert, explore, Scenario, ENDING_LABEL

########################################################

class ListTests(unittest.TestCase):

    ########################################################

    def test_1(self):
        # create a client
        def client1(cl):
            cl.add(1)

        # create another client
        def client2(cl):
            cl.add(2)
        
        class Scenario_1(Scenario):
            
            def testcase(self):
                cl = concurrent_list()
                t1 = self.thread("client1", client1, cl)
                t2 = self.thread("client2", client2, cl)
                
                self.until_star().transfer_star()
                self.until_star().transfer_star()

                print("Cl:", cl)
                Assume(cl.contents() == [2, 1])
                Assert(1 in cl)
                Assert(2 in cl)

                
        scenario = Scenario_1().explore_exists()
        scenario.save("scenario_listtest3_test1")
        
        print("\nPrinting the scenario")
        print(str(scenario))

        scenario.save("scenario1.txt")

        print("\nLoaded scenario")
        print(Scenario.load("scenario1.txt"))




    ########################################################

if __name__ == '__main__':
    unittest.main()