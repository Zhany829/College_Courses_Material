import subprocess

def solve(constraints):
    instance = "p cnf {} {}\n".format(N*K,len(constraints)) + "\n".join( [' '.join(map(str,c)) + ' 0' for c in constraints] )
    # print(instance)
    sat = subprocess.run(["minisat"], stdout=subprocess.PIPE, universal_newlines=True, input=instance)
    return sat.stdout.split('\n')[-2] == 'SATISFIABLE'


def get_line_numbers():
    return list( map(int, input().split(' ') ) )

def get_constraints():
    constraints = []
    for a,b in edges:
        for k in range(K):
            constraints.append([-(a*K-k),-(b*K-k)])
    for i in range(N):
        constraints.append([(i+1)*K-k for k in range(K)])
    return constraints

N,M,K = get_line_numbers()
edges = []
for i in range(M):
    edges.append( get_line_numbers() )


print(solve(get_constraints()) )
