def findRepresentative(a):
  while a != parent[a]: a = parent[a]
  return a

def unionSets(a,b):
  a = findRepresentative(a)
  b = findRepresentative(b)
  if a == b: return False
  if depth[a] > depth[b]:
    parent[b] = a
  elif depth[a] > depth[b]:
    parent[a] = b
  else: # Same Depth
    parent[a] = b
    depth[a] = depth[a] + 1
  return True

def by_cost(elem): return elem[2]
def readTriple():
  a, b, c = input().split(' ')[:3]
  return int(a), int(b), int(c)

N, K, M = readTriple()
edges = sorted( [readTriple() for _ in range(M)], key=by_cost )
parent = [i for i in range(N+1)]
depth = [0 for i in range(N+1)]

remainingEdgesToJoin = N - K
ans = -1
for a,b,c in edges:
  if not unionSets(a,b): continue
  remainingEdgesToJoin -= 1
  if remainingEdgesToJoin == 0:
    ans = c
    break
print(ans)
