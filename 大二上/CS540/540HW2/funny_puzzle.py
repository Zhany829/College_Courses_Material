#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import heapq
# given a state of the puzzle, represented as a single list of integers with a 0 in the empty space, 
# print to the console all of the possible successor states
def print_succ(state):
    successors = get_successor(state)
    for i in range(len(successors)):
        h_value = get_Manhattan_dis(successors[i], [1,2,3,4,5,6,7,8,0])
        successor = successors[i]
        print(str(successor) + str(' h=') + str(h_value))
       
    
#given a state of the puzzle, perform the A* search algorithm and print the path from the current state to the goal state    
def solve(state):
    print(1)
    open_pq = []
    close = {}
    index_g = 0
    parent_index = -1
    key_index = 0
    goal_state = [4, 3, 8, 5, 1, 6, 7, 2, 0]
    res_list = []
    heapq.heappush(open_pq,(get_item_info(state,index_g,get_Manhattan_dis(state, goal_state),parent_index)))
    res_list.append((open_pq[0])[1])
    while not len(open_pq) == 0:
        successors = get_successor((open_pq[0])[1])
        index_g += 1
        parent_index += 1
    for i in range(len(successors)):
        heapq.heappush(open_pq,(get_item_info(successors[i],index_g,get_Manhattan_dis(state, goal_state),parent_index)))
        min_item = heapq.heappop(open_pq)[1]
        res_list.append(min_item)
        close[key_index] = min_item
        key_index += 1
        if min_item == goal_state:
            print_result(res_list)
            break
        print(open_pq)
        
        
        
def get_item_info(state,g,h,parent_index):
    info = ((g+h),state,(g,h,parent_index))
    return info
    
#author: Ajinkya Sonawane
#source:https://blog.goodaudience.com/solving-8-puzzle-using-a-algorithm-7b509c331288
#The following code is obtained the inspiraion from source above.

#Generate successor nodes from the given node by moving the blank space
def get_successor(state):
    #get the position of empty place
    row,column = findEmpty(state,0)
    #neighbor_list contains position values for moving the empty place in either of
    #the 4 directions [up,down,left,right] respectively.
    neighbor_list = [[row,column-1],[row,column+1],[row-1,column],[row+1,column]]
    valid_succ = []
    for i in neighbor_list:
        successors = move(state,row,column,i[0],i[1])
        if successors is not None:
            successor = []
            #generate the valid successor
            for j in range(3):
                for k in range(3):
                    successor.append(successors[j][k])
            valid_succ.append(successor)
    
    sorted_succ = sorted(valid_succ)
    return sorted_succ
        
def move(state,x1,y1,x2,y2):
    # Move the empty place in the given direction and if the position value are out
    # of limits the return None
    if x2 >= 0 and x2 < 3 and y2 >= 0 and y2 < 3:
        successor = []
        successor = copy(state)
        temp = successor[x2][y2]
        successor[x2][y2] = successor[x1][y1]
        successor[x1][y1] = temp
        return successor
    else:
        return None
            
#Copy function to create a similar matrix of the given node
def copy(state):
    ret = []
    for i in range(3):
        ret.append([0 for j in range(3)])
    index = 0
    for i in range(3):
        for j in range(3):
            ret[i][j] = state[index]
            index += 1
    return ret
    
    
#find the empty place in the current mtrix
def findEmpty(state,empty):
        temp = copy(state)
        #Specifically used to find the position of the blank space 
        for i in range(3):
            for j in range(3):
                if temp[i][j] == empty:
                    return i,j
                
#author: AcrobatAHA
#source:https://github.com/AcrobatAHA/How-to-solve-an-8-puzzle-problem-using-A-Algorithm-in-python-/blob/master/Heuristics%20for%20the%208-puzzle.py
#The following code is obtained the inspiraion from source above.

#get the Manhattan distance
def get_Manhattan_dis(oneD_cur_state, oneD_goal_state):
    dis = 0
    cur_state = copy(oneD_cur_state)
    goal_state = copy(oneD_goal_state)
    for i in range(len(cur_state)):
        for j in range(len(cur_state)):
            if cur_state[i][j] == 0:
                continue
            if (goal_state[0][0] == cur_state[i][j]):
                 dis += (abs(i-0) + abs(j-0))
            elif (goal_state[0][1] == cur_state[i][j]):
                dis += (abs(i-0) + abs(j-1))
            elif (goal_state[0][2] == cur_state[i][j]):
                dis += (abs(i-0) + abs(j-2))
            elif (goal_state[1][0] == cur_state[i][j]):
                dis += (abs(i-1) + abs(j-0))            
            elif (goal_state[1][1] == cur_state[i][j]):
                dis += (abs(i-1) + abs(j-1))
            elif (goal_state[1][2] == cur_state[i][j]):
                dis += (abs(i-1) + abs(j-2))
            elif (goal_state[2][0] == cur_state[i][j]):
                dis += (abs(i-2) + abs(j-0))
            elif (goal_state[2][1] == cur_state[i][j]):
                dis += (abs(i-2) + abs(j-1))   
            elif (goal_state[2][2] == cur_state[i][j]):
                dis += (abs(i-2) + abs(j-2))
    return dis

def print_result(lis):
    for i in range(len(lis)):
        manhattan_dis = get_Manhattan_dis(lis[i], [1,2,3,4,5,6,7,8,0])
        print(str(lis[i]) + str(' h=') + str(manhattan_dis) + str(' moves: ') + str(i))


        
print(get_successor([4,3,8,5,1,6,7,2,0]))



    
print(solve([4,3,8,5,1,6,7,2,0]))


# In[ ]:





# In[ ]:




