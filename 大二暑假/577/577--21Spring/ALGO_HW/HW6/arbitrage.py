import math

def checkNegativeCycle(N,M,tax):
    dist = {}
    ok = True
    for i in range(int(N)):
        ok = True
        for i in range(int(M)):
            a,b,c = edges[i]
            if a not in dist: dist[a] = 0
            if b not in dist: dist[b] = 0
            c = - math.log( float(c) * (1-tax) / 100 )
            if dist[b] > dist[a] + c:
                dist[b] = dist[a] + c
                ok = False
    return ok

N,M = input().split(' ')
edges = []
for i in range(int(M)):
    edges.append( input().split(' ') )

for tax in range(100):
    if checkNegativeCycle(N,M,tax / 100.0):
        print(tax)
        break
