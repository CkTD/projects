import math
import random

def kmeans(x, K, dist_func, mean_func):
    # x    data set 
    # K    number of clusters
    # dist_func  dist_func(x[i], x[j]) -> dist
    # mean_func  mean_func([x[i], x[j], x[k], ....]) -> mean x
    # m    size of data set x
    # c[i] index of cluster (1,2,...,K) to which x[i] is currently assigned
    # u[i] average(mean) of points assigned to cluster k(1,2,...,K)
    m = len(x)
    c = [None] * m
    # randomly initialize K cluster centroids u
    u = [x[random.randrange(0,m)] for i in range(K)]
    while True:
        converge = True
        # cluster assignment step
        for i in range(m):
            k, _ = min([(k, dist_func(x[i], u[k])) for k in range(K)], key=lambda x: x[1])
            if c[i] != k:
                converge = False
                c[i] = k
        # check if converge
        if converge:
            break
        # move centroids step
        for k in range(K):
            u[k] = mean_func([x[i] for i in range(m) if c[i] == k])
    return c


def test():
    x = [(10,10), (1,2), (1,1), (2,2), (3,0), (0,3),
         (10,10),(11,10),(10,9),(11,9),(11,12)]
    dist_func = lambda P,Q: math.sqrt((P[0] - Q[0]) ** 2 + (P[1] - Q[1]) ** 2)
    mean_func = lambda x: tuple( ( sum(i)/len(i) for i in zip(*x)) )
    c = kmeans(x,2,dist_func, mean_func)
    print(c)

if __name__ == '__main__':
    test()
