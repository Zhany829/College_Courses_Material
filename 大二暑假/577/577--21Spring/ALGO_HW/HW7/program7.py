import sys

def solve(height,win):
    player = (height+1)%2
    win = [int(c) for c in win]
    value = [1 for __ in win]
    while height:
        height -= 1
        win_new = [0]*(len(win)//2)
        value_new = [0]*(len(value)//2)
        for i in range(0,len(win),2):
            if win[i] == player and win[i+1] == player:
                win_new[i//2] = player
                value_new[i//2] = (value[i]+value[i+1])/2
            elif win[i] == player and win[i+1] != player:
                win_new[i//2] = player
                value_new[i//2] = value[i] + value[i+1]/2
            elif win[i] != player and win[i+1] == player:
                win_new[i//2] = player
                value_new[i//2] = value[i]/2 + value[i+1]
            else:
                win_new[i//2] = (player+1)%2
                value_new[i//2] = value[i] + value[i+1]
        player = (player+1)%2
        value = value_new
        win = win_new
    return (win, value)

height = sys.stdin.readline().strip()
win = sys.stdin.readline().strip()

print(solve(int(height), win)[1][0])
