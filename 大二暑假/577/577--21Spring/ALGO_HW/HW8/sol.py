from array import *
from collections import deque

#Create network from biadjacency matrix
def create_network(l_size, r_size, bi_adj_matrix):
    transp_bi_adj_matrix = list(map(list, zip(*bi_adj_matrix)))
    n_adj_matrix = []
    n_adj_list = []
    n_adj_matrix.append([0] + l_size*[1] + (r_size+1)*[0])
    n_adj_list.append([k for k,x in enumerate(n_adj_matrix[0]) if x == 1])
    for i in range(l_size):
        n_adj_matrix.append((l_size + 1)*[0] + bi_adj_matrix[i] + [0])
        n_adj_list.append([k for k,x in enumerate(n_adj_matrix[i+1]) if x == 1])
    for i in range(r_size):
        n_adj_matrix.append((l_size+r_size+1)*[0] + [1])
        edges_adj_list = [0] + transp_bi_adj_matrix[i] + r_size*[0] + [1]
        n_adj_list.append([k for k,x in enumerate(edges_adj_list) if x == 1])
    n_adj_matrix.append((l_size + r_size + 2)*[0])
    n_adj_list.append([k for k,x in enumerate(n_adj_matrix[l_size + r_size + 1]) if x == 1])
    return (n_adj_list, n_adj_matrix)

#Max Flow using Edmonds-Karp
def max_flow(n_adj_list, n_adj_matrix, n):
    path_parents = breadth_first_search(n_adj_list, n_adj_matrix, n)
    while path_parents != []:
        augment_path(n_adj_matrix, path_parents, n)
        path_parents = breadth_first_search(n_adj_list, n_adj_matrix, n)
    return sum(n_adj_matrix[n-1])

#BFS code for finding a path between s and t, returns [] if not found
def breadth_first_search(n_adj_list, n_adj_matrix, n):
    queue = []
    visited = [0]*n
    queue.append(0)
    parents = n*[0]
    while queue:
        curr = queue.pop(0)
        if curr == n-1:
            return parents
        if visited[curr] == 0:
            for i in n_adj_list[curr]:
                if n_adj_matrix[curr][i] > 0 and visited[i] == 0:
                    queue.append(i)
                    parents[i] = curr
            visited[curr] = 1
    return []
    
#Path augmentation (simplified for bipartite matching)    
def augment_path(n_adj_matrix, parents, n):
    curr = n-1
    while curr != 0:
        nxt = parents[curr]
        n_adj_matrix[curr][nxt] += 1
        n_adj_matrix[nxt][curr] -= 1
        curr = nxt

#Read number of vertices in L and R
[l_size, r_size] = map(int, input().split())

#Read biadjacency matrix for the bipartite graph
bi_adj_matrix = []
for i in range(l_size):
    inputs = list(map(int, input().split()))
    bi_adj_matrix.append(inputs)

#Create network and run Edmonds-Karp
(n_adj_list, n_adj_matrix) = create_network(l_size, r_size, bi_adj_matrix)
m = max_flow(n_adj_list, n_adj_matrix, l_size + r_size + 2)
print(m)
