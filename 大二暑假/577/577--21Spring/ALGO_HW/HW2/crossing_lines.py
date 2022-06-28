# post-Condition: array that's returned is sorted
def sortAndCount(arr):
    if len(arr) < 2:
        return 0, arr
    mid_ind = len(arr)//2
    left_count, arr_left = sortAndCount(arr[:mid_ind])
    right_count, arr_right = sortAndCount(arr[mid_ind:])
    across_halves_count, arr = mergeAndCount(arr_left, arr_right)
    total_count = left_count + right_count + across_halves_count
    return total_count, arr

# pre-condition: arr_left and arr_right are sorted
def mergeAndCount(arr_left, arr_right):
    # count number of inversions between left and right arrays
    # and merge left and right arrays into one sorted array
    cross_count = 0
    l = 0
    r = 0
    sorted_arr = []

    while(l < len(arr_left) and r < len(arr_right)):
        if (arr_right[r] < arr_left[l]):
            cross_count += (len(arr_left) - l)
            sorted_arr.append(arr_right[r])
            r += 1
        else:
            sorted_arr.append(arr_left[l])
            l += 1
    # append remainder
    if l < len(arr_left):
        sorted_arr.extend(arr_left[l:])
    elif r < len(arr_right):
        sorted_arr.extend(arr_right[r:])
    return cross_count, sorted_arr

# get number of points on each line
nlines = int(input())

# get points on top line
points_top = []
for _ in range(nlines):
    points_top.append(int(input()))

# get points on bottom line
points_bottom = []
for _ in range(nlines):
    points_bottom.append(int(input()))

# sort points by order on top line
points = [(points_top[i], points_bottom[i]) for i in range(nlines)]

# basic test case, should print 5
# points = [(2, 5), (1, 1), (4, 2), (3, 4), (5, 3)]
points.sort(key=lambda point: point[0])
count, arr = sortAndCount([points[i][1] for i in range(len(points))])
print(count)
