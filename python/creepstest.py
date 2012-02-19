
import sys
import pygame
sys.path.append('.')
sys.path.append('./creeps_game')

from vec2d import vec2d
from creeps import *
import unittest
from counit import coroutine, Assume, Assert, explore, Scenario


def create_creep(game, init_position, init_direction = (1,1)):
    creep = Creep(screen=game.screen,
                    game=game,
                    creep_images=game.creep_images[0],
                    explosion_images=game.explosion_images,
                    field=game.field_rect,
                    init_position=init_position,
                    init_direction=init_direction,
                    speed=0.05)
    for i in range(0,5):
        creep.update(100)
    
    game.creeps.add(creep)
    game._spawned_creep_count += 1
    return creep
    
class ListTests(unittest.TestCase):
    
    ########################################################

    def test_3(self):
        
        # create a client
        def client1(cl):
            cl.add(1)

        # create another client
        def client2(cl):
            cl.add(2)
        
        class Scenario_1(Scenario):
            
            def teardown(self):
                pygame.quit()
            
            def testcase(self):
                
                # create a game
                game = Game(spawn_creeps=False, create_walls=False)
                
                # create a creep
                c1 = create_creep(game, (105,105))
                c2 = create_creep(game, (105,105))
                c3 = create_creep(game, (105,105))
                
                pc1 = c1.gridpath.clone_path_cache()
                pc2 = c2.gridpath.clone_path_cache()
                pc3 = c3.gridpath.clone_path_cache()
                
                Assert(not c1.gridpath.is_path_cache_empty())
                Assert(not c2.gridpath.is_path_cache_empty())
                Assert(not c3.gridpath.is_path_cache_empty())
                
                game.options['add_wall'] = True
                
                wall1 = (200, 200)
                wall2 = (300, 300)
                
                c1.gridpath.is_path_blocked_by = (lambda curr, query: True if query == game.xy2coord(wall1) else False)
                c2.gridpath.is_path_blocked_by = (lambda curr, query: True if query == game.xy2coord(wall2) else False)
                c3.gridpath.is_path_blocked_by = (lambda curr, query: True if query == game.xy2coord(wall1) else False)
                
                t1 = self.thread("client1", game.on_add_wall, wall1)
                t2 = self.thread("client2", game.on_add_wall, wall2)
                
                ##################################################
                self.until("on_add_wall").transfer(t1)
                ##################################################
                
                Assume(not c1.gridpath.compare_path_cache(pc1))
                Assume(c2.gridpath.compare_path_cache(pc2))
                Assume(c3.gridpath.compare_path_cache(pc3))
                
                Assert(c1.gridpath.is_path_cache_empty())
                
                ##################################################
                self.until_end().transfer(t2)
                ##################################################
                
                Assert(not c1.gridpath.compare_path_cache(pc1))
                Assume(not c2.gridpath.compare_path_cache(pc2))
                Assume(c3.gridpath.compare_path_cache(pc3))
                
                Assert(c2.gridpath.is_path_cache_empty())
                
                ##################################################
                self.until_end().transfer(t1)
                ##################################################
                
                Assert(not c1.gridpath.compare_path_cache(pc1))
                Assert(not c2.gridpath.compare_path_cache(pc2))
                Assert(not c3.gridpath.compare_path_cache(pc3))
                
                Assert(c3.gridpath.is_path_cache_empty())
                
        scenario = Scenario_1().explore_exists()
        scenario.save("scenario_creeptest3")
    
    ########################################################

    def __test_2(self):
        
        # create a client
        def client1(cl):
            cl.add(1)

        # create another client
        def client2(cl):
            cl.add(2)
        
        class Scenario_1(Scenario):
            
            def teardown(self):
                pygame.quit()
            
            def testcase(self):
                
                # create a game
                game = Game(spawn_creeps=False, create_walls=False)
                
                # create a creep
                c1 = create_creep(game, (105,105))
                    
                pc = c1.gridpath.clone_path_cache()
                Assert(not c1.gridpath.is_path_cache_empty())
                
                Assert(c1.gridpath.compare_path_cache(pc))
                
                game.options['add_wall'] = True
                t1 = self.thread("client1", game.on_add_wall, (200, 200))
                self.until_end().transfer(t1)
                
                Assert(c1.gridpath.is_path_cache_empty())
                Assert(not c1.gridpath.compare_path_cache(pc))
                
        scenario = Scenario_1().explore()
        scenario.save("scenario_creeptest2")
        

    ########################################################

    def __test_1(self):
        
        # create a client
        def client1(cl):
            cl.add(1)

        # create another client
        def client2(cl):
            cl.add(2)
        
        class Scenario_1(Scenario):
            
            def teardown(self):
                pygame.quit()
            
            def testcase(self):
                
                # create a game
                game = Game(spawn_creeps=False, create_walls=False)
                
                # create a creep
                c = create_creep(game, (100,100))
                
                c._point_is_inside = (lambda p: True) # whatevet point we give, it is inside
                
                t1 = self.thread("client1", c.mouse_click_event, (110, 110))
                
                self.until_end().transfer(t1)
                
                Assert(c.health == 12)

                print("Handler ended!")
                
        scenario = Scenario_1().explore()
        scenario.save("scenario_creeptest1")
        

    ########################################################

if __name__ == '__main__':
    unittest.main()