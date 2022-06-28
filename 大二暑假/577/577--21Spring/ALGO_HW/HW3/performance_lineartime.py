import collections

def check(path, paths):
  path = set(path)
  if len(path) == 1:
    if len(paths) > 4: return False
  if len(path) == 2:
    if len(paths) > 12: return False
    a,b = path
    if len([1 for p in paths if a in p]) > 3: return False
    if len([1 for p in paths if b in p]) > 3: return False
  if len(path) == 3 and len(paths) > 0:
    ok = False
    for v in paths[0]:
      if v in path: continue
      rem = [p for p in paths if v not in p]
      if len(rem) == 0:
        ok = True
        break
      ok |= len( set.intersection(*rem).difference(path) ) > 0
    if not ok: return False
  if len(path) == 4 and len(paths) > 0:
    ok = len( set.intersection(*paths).difference(path) ) > 0
    if not ok: return False
  paths.append( path )
  return True

def next(A,a):
  candidates = []
  for b in A[a]:
    if b in [a]: continue
    if not check([a],paths[1][b]): continue
    for c in A[b]:
      if c in [a,b]: continue
      if not check([a,b],paths[2][c]): continue
      for d in A[c]:
        if d in [a,b,c]: continue
        if not check([a,b,c],paths[3][d]): continue
        for e in A[d]:
          if e in [a,b,c,d]: continue
          if not check([a,b,c,d],paths[4][e]): continue
          for f in A[e]:
            if f in [a,b,c,d,e]: continue
            candidates.append(f)
  return candidates

def bfs(A, source, sink):
  Q = collections.deque([0])
  dist = {source: 0}
  while Q:
    u = Q.popleft()
    candidates = next(A,u)
    for w in candidates:
      if w in dist: continue
      dist[w] = dist[u] + 1
      Q.append(w)
      if sink in dist: return dist[sink]
  return -1

def PerformanceShow(E):
  A = [[] for _ in range(100000)]
  N = 1
  for a,b in E:
    A[a-1].append(b-1)
    A[b-1].append(a-1)
    if a > N: N = a
    if b > N: N = b
  return bfs(A,0,N-1)

def readPair():
    a, b = input().split(' ')
    return int(a), int(b)

N, M = readPair()
paths = [[[] for i in range(N)] for l in range(5)]
E = [ readPair() for _ in range(M) ]
print( PerformanceShow(E) )
