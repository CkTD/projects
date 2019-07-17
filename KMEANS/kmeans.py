import math
import random

def kmeans_one(x, K, dist_func, mean_func):
    # x    data set 
    # K    number of clusters
    # dist_func  dist_func(x[i], x[j]) -> dist
    # mean_func  mean_func([x[i], x[j], x[k], ....]) -> mean x
    # m    size of data set x
    # c[i] index of cluster (1,2,...,K) to which x[i] is currently assigned
    # u[i] average(mean) of points assigned to cluster k(1,2,...,K)
    # J    Optimization objective(distortion cost function)
    m = len(x)
    c = [None] * m
    
    # randomly initialize K cluster centroids u
    u = random.sample(x, K)
    
    while True:
        converge = True
        # cluster assignment step (update c)
        for i in range(m):
            k, _ = min([(k, dist_func(x[i], u[k])) for k in range(K)], key=lambda x: x[1])
            if c[i] != k:
                converge = False
                c[i] = k
        # check if converge
        if converge:
            break
        # move centroids step (update u)
        for k in range(K):
            xc = [x[i] for i in range(m) if c[i] == k] # slow!!
            u[k] = mean_func(xc) if xc else random.sample(x, 1)[0]
    
    J = 1/m * sum([dist_func(x[i], u[c[i]]) for i in range(m)])
    
    return c, J

def kmeans(x, K, dist_func, mean_func, iter):
    # run k-means with multiple random initializations and pick best one.
    return min([kmeans_one(x,K,dist_func,mean_func) for i in range(iter)], 
               key=lambda x:x[1])

def test():
    import matplotlib.pyplot as plt
    import numpy as np

    m = 50
    d_x = np.random.rand(m)
    d_y = np.random.rand(m) + d_x
    x = list(zip(d_x, d_y))
    #x = [(10,10), (1,2), (1,1), (2,2), (3,0), (0,3),
    #     (10,10),(11,10),(10,9),(11,9),(11,12)]
    dist_func = lambda P,Q: math.sqrt((P[0] - Q[0]) ** 2 + (P[1] - Q[1]) ** 2)
    mean_func = lambda x: tuple( ( sum(i)/len(i) for i in zip(*x)) )
    K = 4
    c, J = kmeans(x, K, dist_func, mean_func, 100)
    print("CLUSTER:", c)
    print("COST: ", J)
    
    #d_x = [i[0] for i in x]
    #d_y = [i[1] for i in x]
    colors = np.random.rand(K)
    d_c = [colors[i] for i in c]
    plt.scatter(d_x, d_y, c=d_c)
    plt.show() 
if __name__ == '__main__':
    test()
