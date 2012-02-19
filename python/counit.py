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


import os
import copy
from random import random, seed
import stackless


##################################################################
# constants
STARTING_LABEL = "starting"
FINISHING_LABEL = "finishing"
ENDING_LABEL = "ending"
MAIN_LABEL = "main"

##################################################################

def abstractmethod(f):
    def ff(*args, **kwargs):
        raise RuntimeError("Method " + f.__name__ + " not implemented!")
    return ff

##################################################################

def Check(condition, is_assume, msg = ""):
    if is_assume:
        if not condition:
            raise BacktrackException("Assumption failure: " + msg)
    else:
        assert condition, msg

##################################################################

def Assume(condition, msg = ""):
    Check(condition, True, msg)

def AssumeTrue(condition, msg = ""):
    Assume(condition, msg)

def AssumeFalse(condition, msg = ""):
    Assume(not condition, msg)

def AssumeEqual(s1, s2, msg = ""):
    Assume(s1 == s2, "Expected %s, found %s" % (s1,s2) if msg == "" else msg)

def AssumeList(lst):
    assert isinstance(lst, list)
    lst.reverse() # since we will use pop()
    return (lambda elt, lst=lst: AssumeEqual(str(lst.pop()), str(elt)))

##################################################################

def Assert(condition, msg = ""):
    Check(condition, False, msg)

def AssertTrue(condition, msg = ""):
    Assert(condition, msg)

def AssertFalse(condition, msg = ""):
    Assert(not condition, msg)

def AssertEqual(s1, s2, msg = ""):
    Assert(s1 == s2, "Expected %s, found %s" % (s1,s2) if msg == "" else msg)

def AssertList(lst):
    assert isinstance(lst, list)
    lst.reverse() # since we will use pop()
    return (lambda elt, lst=lst: AssertEqual(str(lst.pop()), str(elt)))


##################################################################

def expect_true(cond, msg = "Assertion failed!"):
    if not cond:
        raise RuntimeError(msg)

def expect_false(cond, msg = "Assertion failed!"):
    if cond:
        raise RuntimeError(msg)

def expect_equals(s1, s2):
    expect_true(s1 == s2, "Expected %s, found %s" % (s1,s2))


# returns a function when applied with an element checks if that element was expected in the right order
def expect_list(lst):
    assert isinstance(lst, list)
    lst.reverse() # since we will use pop()
    return (lambda elt, lst=lst: expect_equals(str(lst.pop()), str(elt)))


##################################################################

def explore(setup_functions, threads, teardown_functions, scenario = None, save = False, search_if_fails=False):
    if scenario != None and isinstance(scenario, str):
        filename = scenario
        if os.path.exists(filename):
            scenario = Scenario.load(filename=filename)
        else:
            scenario = Scenario(mode=Scenario.EXPLORE, filename=filename)
    else:
        assert scenario == None or isinstance(scenario, Scenario)
        if scenario == None:
            scenario = Scenario(mode=Scenario.EXPLORE)
    
    assert threads != None
    if type(threads) == list:
        for t in threads:
            scenario.add_thread(t)
    else:
        scenario.test_function = threads

    for f in setup_functions:
        scenario.add_setup(f)

    for f in teardown_functions:
        scenario.add_teardown(f)

    scenario.run(search_if_fails)

    if save:
        assert scenario != None
        assert scenario.filename != None
        scenario.save()
    
    return scenario

##################################################################

class BacktrackException(Exception):
    pass

class NoFeasibleExecutionException(Exception):
    pass

class ReplayFailureException(Exception):
    pass

##################################################################

class TransferListener:
    @abstractmethod
    def on_transfer(self, curr, label, target):
        pass

    @abstractmethod
    def on_yield(self, curr, label, target):
        pass
    
##################################################################

class Scenario(TransferListener):
    
    # scenario modes
    (REPLAY, EXPLORE) = (0, 1)
    
    def __init__(self, 
                 setup_functions = [], 
                 test_function = None,
                 teardown_functions = [],
                 mode = EXPLORE,
                 actions = [], 
                 filename = None):
        self.filename = filename
        self.test_function = test_function
        self.setup_functions = setup_functions
        self.teardown_functions = teardown_functions
        self.group = coroutine_group("Client group")
        self.group.set_scenario(self)
        self.actions = actions
        self.mode = mode
        self.untils = []
        self.preprocess_actions()
        self.index = 0
        self.until_open = False
        self.num_paths = 0
        
            
    def add_thread(self, thread):
        self.group.add(thread, restart=True)

    def add_setup(self, f):
        self.setup_functions.append(f)

    def add_teardown(self, f):
        self.teardown_functions.append(f)

    def setup(self):
        for f in self.setup_functions:
            f()

    def teardown(self):
        for f in self.teardown_functions:
            f()
            
    def thread(self, id, f, *args, **kwargs):
        c = None
        if self.group.has_member(id):
            c = self.group.get_member(id)
            # update function and arguments
            c.update_args(f, *args, **kwargs)
            c.restart()
        else:
            assert isinstance(id, str)
            c = coroutine(f, id, self.group)(*args, **kwargs)
            self.group.add(c, restart=False)
            assert self.group.has_member(id)
        assert c != None
        return c
        
    def until_star(self):
        return self.until(None)
        
    def until_end(self):
        return self.until(ENDING_LABEL)
            
    def until(self, condition):
        if condition == None:
            self.untils = []
        else:
            if isinstance(condition, str):
                # create a checker for the label
                condition = (lambda map, yield_label=condition: map['yield_label'] == yield_label)
            self.untils.append(condition)
            
        self.until_open = True
        return self
        
    def transfer_star(self):
        return self.transfer(None)
        
    def transfer(self, target):
        if not self.until_open:
            self.until_star()
        assert self.until_open
        self.until_open = False
        
        # if the target is not in our group, add it
        assert target == None or self.group.has_member(target) #self.group.add(target)
        # go to target
        assert coroutine.current == coroutine.main
        if target != None and target.ended:
            raise BacktrackException("Target ended!")
        coroutine.transfer(target, label=MAIN_LABEL, group=self.group)
        return self
        
    def run(self, search_if_fails=False):
        if self.mode == Scenario.REPLAY:
            self.replay(search_if_fails)
        elif self.mode == Scenario.EXPLORE:
            self.explore_exists() 
        else:
            raise RuntimeError("Invalid mode")
    
    def run_testcase(self):
        self.try_run(self.setup)

        self.try_run(self.testcase)

        self.try_run(self.teardown)
            
    # ensure that f does not take any argument or does not return any value 
    def try_run(self, f):
        self.group.exception = None
        f()
        if self.group.exception != None:
            raise self.group.exception
    
    def testcase(self):
        try:
            if self.test_function != None:
                self.test_function()
            else:
                while not self.group.all_ended():
                    # transfer to a thread
                    self.transfer(None)
        
        except BacktrackException as e:
            self.group.exception = e
            return
        
    def replay(self, search_if_fails=False):
        self.untils = []
        self.num_paths = 0
        group = self.group
        group.reset()
        try:
            self.run_testcase()

        except BacktrackException:
            print("Some assumption does not hold. Scenario should change!")
            if search_if_fails:
                print("Searching for another path...")
                self.restart()
                self.explore()
                return
            else:
                raise ReplayFailureException()
        
        print("Scenario replayed successfully!")
        
        self.preprocess_actions(reset_actions=True)
        self.paths = 1
        self.untils = []
        group.reset()
        
        print(self.__str__())
        return self


    def explore_exists(self):
        self.mode = Scenario.EXPLORE
        self.actions = []
        self.index = 0
        ##################
        self.untils = []
        self.num_paths = 0
        group = self.group
        group.reset()
        while True:
            try:
                self.run_testcase()
                
                break # if everything is fine, we break the loop
            
            except BacktrackException as e:
                print("Backtracking due to " + e.message)
                if not self.backtrack():
                    raise NoFeasibleExecutionException()
                        
        print("Exploration completed successfully!")
        
        self.preprocess_actions(reset_actions=True)
        self.num_paths = 1
        self.untils = []
        group.reset()
                
        print(self.__str__())
        return self

    def explore_forall(self):
        self.mode = Scenario.EXPLORE
        self.actions = []
        self.index = 0
        ##################
        self.untils = []
        self.num_paths = 0
        group = self.group
        group.reset()
        while True:
            try:
                self.run_testcase()
                
                self.num_paths += 1
                
                raise BacktrackException("Retrying another path")
            
            except BacktrackException as e:
                print("Backtracking due to " + e.message)
                if not self.backtrack():
                    # no more execution exists
                    break
        
        if self.num_paths == 0:
            raise NoFeasibleExecutionException()
        
        print("Exploration completed successfully!")
        print("Explored %d paths." % self.num_paths)
            
        self.preprocess_actions(reset_actions=True)
        self.num_paths = 1
        self.untils = []
        group.reset()
                
        print(self.__str__())
        return self

    def preprocess_actions(self, reset_actions = False):
        self.index = 0
        if len(self.actions) > 0:
            # reset the scenario
            # clear the non-taken transfers
            actions = []
            for action in self.actions:
                if action.taken:
                    assert action.count >= 1
                    if reset_actions:
                        action.rem_count = action.count
                    actions.append(action)
            self.actions = actions

    def get_action(self, i):
        assert len(self.actions) > 0
        assert 0 <= i < len(self.actions)
        action = self.actions[i]
        assert action != None
        return action

    def remove_action(self, i):
        assert len(self.actions) > 0
        assert 0 <= i < len(self.actions)
        action = self.pop(i)
        assert action != None
        return action

    def has_current_action(self):
        return (len(self.actions) > 0) and (0 <= self.index < len(self.actions))

    def current_action(self):
        assert self.has_current_action()
        return self.get_action(self.index)

    def consume_current_action(self):
        action = self.current_action()
        assert action != None
        if action.consume_once():
            self.index += 1
            return True
        return False

    def add_transfer_action(self, curr, label, target, old_targets = [], set_index=False, count=1, free = True):
        assert isinstance(label, str)
        # curr and target can be string or coroutine
        action = TransferAction.makeTransfer(curr, label, target, old_targets, count=count, free=free)
        self.actions.append(action)
        if set_index:
            self.index = len(self.actions)-1
        return action

    def on_transfer(self, curr, label, target):
        assert 0 <= self.index <= len(self.actions)
        action = self.actions[self.index-1]
        assert coroutine.get_desc(curr) == action.source_label
        assert label == action.yield_label
        assert coroutine.get_desc(target) == action.target_label

    def on_yield(self, curr, label, target):
        if self.mode != Scenario.REPLAY:
            # add a non-taken transfer action
            # check the current one first
            assert self.index >= len(self.actions) or self.current_action().taken
            i = self.index - 1
            if i >= 0:
                action = self.get_action(i)
                assert action.target_label == coroutine.get_desc(curr) or \
                       (action.source_label == coroutine.get_desc(curr) and not action.taken)
                if (not action.taken) and (action.yield_label == label):
                    action.count += 1
                else:
                    free = (target == None or target == coroutine.main)
                    action = TransferAction.makeYield(curr, label, target, free=free)
                    self.actions.insert(self.index, action)
                    self.index += 1

    def get_next_action(self, group, curr, label, target):
        assert group != None and isinstance(group, coroutine_group)

        # if we have the current action, then use it
        if 0 <= self.index < len(self.actions):
            action = self.actions[self.index]
            assert action.taken
            
            if self.checkAndResetUntils({'yield_label':label}, taken=True):            
                if action.target_label == None:
                    assert action.free
                    # decide the next target
                    # choose the first action
                    target = group.get_next_running(curr, action.old_targets)
    
                    if target == None:
                        raise BacktrackException()
                    
                    action.target_label = coroutine.get_desc(target)
            else:
                raise BacktrackException("Until condition does not match with existing action!")
            return action
        ##############################################################################
        else: 
            # no next action is found, find out the next target and create the next action
            # returns None if we want curr to continue without yielding
            assert self.mode == Scenario.EXPLORE
            assert self.index == len(self.actions)
            
            if curr == coroutine.main:
                # choose another running coroutine
                free = (target == None)
                if target == None:
                    target = group.get_next_running()
                assert target != coroutine.main
                if target == None or target.ended:
                    raise BacktrackException("Requested target or all ended!")
                return self.add_transfer_action(curr, label, target, free=free, set_index=True)
            elif label != ENDING_LABEL:
                # None target is only when curr is main
                assert target != None and label != None
                # if target is not main, we let curr to yield to the given coroutine
                if target != coroutine.main:
                    assert coroutine.is_valid(target) and label != None
                    return self.add_transfer_action(curr, label, target, free=False, set_index=True)
                else:
                    # target is main
                    if self.checkAndResetUntils({'yield_label':label}):
                        # do transfer
                        assert target != None and label != None
                        return self.add_transfer_action(curr, label, target, free=False, set_index=True)
                    else:
                        # choose to continue if not ending
                        return None # do not yield yet
            else: 
                # curr is not main, and (target is main or otherwise ending label)
                # go to given target (usually main)
                if self.checkAndResetUntils({'yield_label':label}, ending=True):
                    assert coroutine.is_valid(target) and label != None
                    return self.add_transfer_action(curr, label, target, free=False, set_index=True)
                else:
                    raise BacktrackException("Exists unsatisfied conditions!")
        
        assert False, "Unreachable"

    
    def checkAndResetUntils(self, map, ending=False, taken=False):
        assert self.untils != None and isinstance(self.untils, list)
        if not self.hasUntils():
            return ending or taken
        for f in self.untils:
            if not f(map):
                return False
        self.untils = []
        return True
            
            
    def hasUntils(self):
        assert self.untils != None and isinstance(self.untils, list)
        return len(self.untils) > 0

    # None return value == do not yield
    def decide_next_target(self, group, curr, label, target):
        assert curr == coroutine.main or coroutine.is_valid(target)

        action = self.get_next_action(group, curr, label, target)
        if action == None:
            return None # do not yield yet
        

        print()
        print("Current Request: ", coroutine.get_desc(curr), label, coroutine.get_desc(target) if target != None else "None")
        print("Selected Action: ", str(action))


        # we should be at the same thread as the next action points to
        assert action.source_label == coroutine.get_desc(curr)

        # check if the label requested is the same as the next label in the action
        # if labels are different, we tell the thread not to yield yet
        if label == action.yield_label:
            if self.consume_current_action():
                return action.target_label
        #default
        return None # do not yield yet

    def backtrack(self):

        print("\nScenario before backtrack:")
        print(self.__str__())

        assert 0 <= self.index <= len(self.actions)
        while True:
            if len(self.actions) == 0:
                return False # failed to find an alternative path

            action = self.actions.pop()

            if action.target_label != None: # if target is None, this means we could not select a target, so discard it
                if action.taken:
                    # we ignore the actions whose target is main (there are not choice point there)
                    if action.target_label != MAIN_LABEL:
                        # try to choose another target
                        if action.free: # we can change the target only when the action is free
                            print("Adding backtrack from %s, old target is %s" % (action.source_label, action.target_label))
                            old_targets = list(action.old_targets)
                            old_targets.append(action.target_label)
                            count = action.count-1 if action.count > 1 else 1
                            self.add_transfer_action(action.source_label, action.yield_label, None, old_targets, count=count, free=True) # will decide the target later
                            break
                else:
                    assert action.source_label != MAIN_LABEL and action.target_label == MAIN_LABEL # usually yields are from non-main to main
                    if action.free: # we can change the target only when the action is free
                        assert len(action.old_targets) == 0
                        assert action.count >= 1
                        count = action.count-1 if action.count > 1 else 1
                        self.add_transfer_action(action.source_label, action.yield_label, MAIN_LABEL, count=count, free=True) # will decide the target later
                        break
                    
            else:
                # target is none, so discard the last transfer action, because there was no more choice there
                assert action.taken
                continue
            

        # end while
        assert len(self.actions) >= 0

        self.restart()
        
        print("Scenario after backtrack:")
        print(self.__str__())

        return True

    def restart(self):
        # reset the scenario
        self.preprocess_actions(reset_actions=True)

        # restart the threads
        self.group.restart()
        
        # delete untils
        self.untils = []

    
    def __str__(self):
        s = ""
        for action in self.actions:
            s += str(action) + "\n"
        return s

    def save(self, filename = None):
        assert filename != None or self.filename != None
        if filename == None:
            filename = self.filename
        else:
            self.filename = filename
        self.preprocess_actions()
        with open(filename, 'w') as outfile:
            outfile.write(str(self))

    @staticmethod
    def load(filename):
        actions = []
        with open(filename, 'r') as infile:
            for line in infile:
                action = TransferAction.parse(line)
                actions.append(action)
        return Scenario(actions=actions, mode=Scenario.REPLAY, filename=filename)


##################################################################
class TransferAction:
    def __init__(self, source_label, yield_label, target_label, old_targets = [], count = 1, taken = True, free = True):
        self.source_label = source_label
        self.yield_label = yield_label
        self.target_label = target_label
        self.old_targets = list(old_targets)
        assert count >= 1
        self.count = count
        self.rem_count = count
        self.taken = taken
        self.free = free # false means target cannot change in exploration

    # return True if this is consumed completely
    def consume_once(self):
        assert self.rem_count >= 1
        self.rem_count -= 1
        return self.rem_count == 0

    @staticmethod
    def makeTransfer(curr, label, target, old_targets = [], count = 1, free = True):
        return TransferAction(coroutine.get_desc(curr),
                              label,
                              coroutine.get_desc(target) if target != None else None,
                              old_targets,
                              count=count,
                              free=free,
                              taken = True)
    @staticmethod
    def makeYield(curr, label, target, old_targets = [], count = 1, free = True):
        return TransferAction(coroutine.get_desc(curr),
                              label,
                              coroutine.get_desc(target) if target != None else None,
                              old_targets,
                              count=count,
                              free=free,
                              taken = False)

    def __str__(self):
        return "%s, %s, %d -> %s" % (self.source_label,
                                     self.yield_label,
                                     self.count,
                                     self.target_label)

    @staticmethod
    def parse(s):
        parts = s.split("->")
        source = parts[0].split(",")
        return TransferAction.makeTransfer(source[0].strip(),
                                           source[1].strip(),
                                           parts[1].strip(),
                                           count=int(source[2].strip()))
        
##################################################################


class coroutine_group(TransferListener):
    def __init__(self, name):
        self.name = name
        self.members = {}
        self.memberlist = []
        self.next_id = 0
        self.exception = None
        self.members_to_restart = []
        

    def __iter__(self):
        return self.members.values().__iter__()

    def size(self):
        return len(self.memberlist)

    def reset(self):
        self.exception = None
        coroutine.current = coroutine.main

    def restart(self):
        for t in self.members_to_restart:
            t.restart()
        self.reset()

    def set_scenario(self, scenario):
        self.scenario = scenario

    def assign_id(self):
        id = self.next_id
        self.next_id += 1
        return "co" + str(id)

    def add(self, c, restart=True):
        if c.desc == None:
            c.desc = self.assign_id()
        assert isinstance(c, coroutine)
        self.members[c.desc] = c
        c.group = self
        self.memberlist.append(c)
        if restart:
            self.members_to_restart.append(c)

    def has_member(self, desc):
        desc = coroutine.get_desc(desc)
        assert desc != None and desc != MAIN_LABEL
        return desc in self.members

    def get_member(self, desc):
        assert desc != None
        if desc == MAIN_LABEL:
            return coroutine.main
        assert desc in self.members
        return self.members[desc]

    def all_ended(self):
        for t in self:
            if not t.ended:
                return False
        return True

    def get_next_running(self, curr = None, old_targets = []):
        # ensure that curr is not main
        if curr == coroutine.main:
            curr = None
        if self.all_ended():
            return None
        i = -1
        if curr != None:
            for j in range(0, len(self.memberlist)):
                if self.memberlist[j] == curr:
                    i = j
                    break
        assert i >= -1 and (curr == None or self.memberlist[i] == curr)
        i += 1
        if i >= len(self.memberlist):
            i = 0
        while True:
            assert self.memberlist[i] != None, "None member in group!"
            if self.memberlist[i] == curr:
                return None # means all threads ended
            elif old_targets.count(coroutine.get_desc(self.memberlist[i])) > 0 or self.memberlist[i].ended:
                if curr == None:
                    curr = self.memberlist[i] # to avoid infinite searches
                i += 1
                if i >= len(self.memberlist): 
                    i = 0
            else:
                return self.memberlist[i]
            

    # method to determine where to go from curr-label, target is the current target but may change
    def decide_next_target(self, curr, label, target):
        assert curr == coroutine.main or coroutine.is_valid(target)
        # if we have a scenario, ask the scenario
        if self.scenario != None:
            target = self.scenario.decide_next_target(self, curr, label, target)
            if target == None: return None # do not yield yet
            if isinstance(target, str):
                target = self.get_member(target)

        # otherwise return the given one
        assert coroutine.is_valid(target)
        return target

    def on_transfer(self, curr, label, target):
        if self.scenario != None:
            self.scenario.on_transfer(curr, label, target)

    def on_yield(self, curr, label, target):
        if self.scenario != None:
            self.scenario.on_yield(curr, label, target)


##################################################################
class coroutine:
    "Coroutine class"

    # currently executing coroutine
    main = object() # this marks the main target
    current = main
    # global channel of main to communicate with other coroutines
    channel = stackless.channel()
    channel.preference = 1 # prefer sender (sender does not block)

    @staticmethod
    def is_valid(target):
        return target != None and (target == coroutine.main or isinstance(target, coroutine))

    def __init__(self, function, desc = None, group = None, ctxbound = 0):
        assert function != None
        self.function = function
        # the source of the control from to this coroutine
        self.caller = None
        self.started = False
        self.ended = False
        
        self.desc = desc
        self.ctxbound = ctxbound # context bound: 0 means unbounded, 1 means sequential, > 1 means bounded
        self.scenario = None

        self.args = None
        self.kwargs = None
        
        assert group == None or not group.has_member(self) # we do the addition externally
        self.group = group

    # the first call causes the initialization
    # the other calls transfer to the coroutine
    def __call__(self, *args, **kwargs):
        if not self.started:
            # create and run the tasklet
            assert self != None
            assert args != None and kwargs != None
            self.update_args(self.function, *args, **kwargs)
            self.tasklet = stackless.tasklet(self.tasklet_function)(*args, **kwargs)
            self.channel = stackless.channel()
            self.channel.preference = 1 # prefer sender (sender does not block)
            self.started = True
            if self.desc == None and self.group != None:
                self.desc = self.group.assign_id()
            return self
        else:
            assert self != None
            return coroutine.transfer(self, label=STARTING_LABEL, internal=True)


    def update_args(self, function, *args, **kwargs):
        self.function = function
        self.args = args # copy.copy(args)
        self.kwargs = kwargs # copy.copy(kwargs)

    def restart(self):
        self.caller = None
        self.started = False
        self.ended = False
        
        args = self.args if self.args != None else []
        kwargs = self.kwargs if self.kwargs != None else {}
        return self.__call__(*args, **kwargs) # restarts the coroutine

    @staticmethod
    def get_desc(co):
        assert co != None
        if co == coroutine.main:
            return MAIN_LABEL
        if isinstance(co, str):
            return co
        assert isinstance(co, coroutine)
        return co.desc

    def receive(self):
        # if ended, just return, do not wait for receiving something
        if self.ended:
            self.caller = None
        else:
            assert self.channel != None
            self.caller = self.channel.receive()
        return self.caller

    def send(self, msg):
        assert self.channel != None
        return self.channel.send(msg if msg == coroutine.main or (msg != None and not msg.ended) else None)

    @staticmethod
    def send_to_main(msg):
        assert msg != coroutine.main
        coroutine.channel.send(msg if (msg != None and not msg.ended) else None)

    @staticmethod
    def receive_from_main():
        return coroutine.channel.receive()

    def tasklet_function(self, *args, **kwargs):
        assert self != None
        # block immediately
        self.receive() # make sure that this is not a call to transfer

        try:

            # run the actual function
            self.function(*args, **kwargs)

        except BacktrackException as e:
            # go back to the main caller
            assert self.group != None
            self.group.exception = e # store the exception so that main can decide why we return there
            assert self == coroutine.current
            self.ended = True
            coroutine.transfer(label=ENDING_LABEL, ending=True, internal=True)
            return

        # send caller something to awake its receive and indicate that this is dead
        # note: sent to main if caller is None
        assert self == coroutine.current
        self.ended = True
        print("Coroutine %s is ending." % self.desc)
        coroutine.transfer(None, label=ENDING_LABEL, ending=True) # Note: internal is False
        return



    # run the coroutine sequential till the end of the computation (ignore transfers)
    def finish(self):
        assert self != None and self.started
        if not self.ended:
            self.ctxbound = 1 # makes it sequential
            # TODO(elmas): what if self transfers to another coroutine!? (is it skipped, or run sequentially as well?)
            coroutine.transfer(self, label=FINISHING_LABEL, internal=True)
            assert self.ended


    # transfer to another coroutine
    # target is another coroutine or None
    # if target == None, then this means we want to go back to the caller thread
    # if target == coroutine.main, then this means we want to go back to the main thread
    # returns the coroutine from which the control flow is coming
    # in the case of calling this from main, returns None,
    # in the case of not transferring, returns the current coroutine
    # if internal=True, do not notify transfer listeners
    @staticmethod
    def transfer(target = main, label = None, ending = False, internal = False, group = None):
        assert internal or label != None, "Non-internal transfer without a label!"

        curr = coroutine.current
        assert curr != None
        if curr != coroutine.main:
            assert ending or not curr.ended
        assert not ending or label == "ending"

        #############################

        # decide the target
        if target == coroutine.main: # means to go to main stack
            # if we are already there, just return
            if curr == coroutine.main or curr.tasklet == stackless.main:
                return curr
        elif target == None:
            if curr != coroutine.main:
                target = curr.caller
                if target == None:
                    target = coroutine.main
        else:
            assert target.started and not target.ended

        assert curr == coroutine.main or target != None
        #############################
        
        # after target is explicit, then ask the group about what to do
        # we give the current target and label and ask group where to go next
        if group == None:
            if curr != coroutine.main and curr.group != None:
                group = curr.group
            elif target != coroutine.main and target.group != None:
                group = target.group

        if not internal and group != None:
            new_target = group.decide_next_target(curr, label, target)
            if new_target == None: # means do not transfer yet
                # let others see the (non-taken) yield point
                if group != None: group.on_yield(curr, label, target)
                return curr # group wants us to not transfer

            target = new_target
        
        assert target != None
        #############################

        # check the context bound
        # should not be ending because we should really transfer when ending 
        if not ending and curr != coroutine.main:
            assert curr.ctxbound >= 0
            if curr.ctxbound > 0: # if bound is 0 do not care
                if curr.ctxbound == 1:
                    return curr # reached the bound, ignore the transfer
                else:
                    assert curr.ctxbound > 1
                    curr.ctxbound -= 1

        #############################

        # perform the actual transfer
        coroutine.current = target
        if target == coroutine.main:
            # transfer to the caller or main computation
            # we must be in other than main
            assert stackless.current != stackless.main
            assert curr != coroutine.main # current coroutine cannot be null

            print("Transferring to TOP")

            coroutine.send_to_main(curr) # send main from where the control flow is coming
            if not internal and group != None: group.on_transfer(curr, label, coroutine.main)
            return curr.receive()

        else:
            print("Transferring to ", target.desc)
            if target.ended == True:
                raise RuntimeError("Target %s is terminated!" % target.desc)
            
            # send message to the coroutine we want to awake
            target.send(curr)

            if curr != coroutine.main:
                # curr is not None, so we must be in other than main
                assert stackless.current != stackless.main
                if not internal and group != None: group.on_transfer(curr, label, target)
                return curr.receive()
            else:
                # curr is main, so we must be in main
                assert stackless.current == stackless.main
                if not internal and group != None: group.on_transfer(curr, label, target)
                # run the stackless scheduler
                stackless.run()
                # here there is no caller to assign to, so we wait on main's channel
                return coroutine.receive_from_main()


# end class coroutine
##################################################################
