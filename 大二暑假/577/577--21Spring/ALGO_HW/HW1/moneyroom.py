def BestPrefixSum(A):
  S = [0]
  for x in A: S.append( S[-1] + x )
  return max(S)

def BestSuffixSum(A):
  return BestPrefixSum(reversed(A))

def MoneyRoom(A):
  if len(A) == 1: return A[0]
  m = len(A) // 2

  # Compute solutions in the first and second half
  ans = max(MoneyRoom(A[:m]), MoneyRoom(A[m:]))

  # Find the solution starting in the first half and ending in the second
  sol = BestSuffixSum(A[:m]) + BestPrefixSum(A[m:])
  return max(ans, sol)

n = int(input())
A = []
for i in range(n):
  A.append( int(input()) )
print(MoneyRoom(A))
