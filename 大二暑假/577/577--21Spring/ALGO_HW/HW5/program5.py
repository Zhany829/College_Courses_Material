coins = int(input())
values = [0]*coins
for i in range(coins):
    values[i] = int(input())

if sum(values)%2:
    print("F")
    exit()

sat_array = []
for __ in range(coins+1):
    sat_array += [[True] + [False]*(sum(values)//2)]

for value in range(1,sum(values)//2+1):
    for coin in range(1,coins+1):
        if sat_array[coin-1][value]:
            sat_array[coin][value] = True
        elif value-values[coin-1]>=0 and sat_array[coin-1][value-values[coin-1]]:
            sat_array[coin][value] = True

if sat_array[coins][sum(values)//2]:
    print("T")
else:
    print("F")
