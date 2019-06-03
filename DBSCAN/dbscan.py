import math

def dbscan(points, dist_func, eps, min_pts):
    NOISE = 0
    def range_query(points, dist_func, point, eps, min_pts):
        neighbors = []
        for P in points:
            if dist_func(P, point) <= eps:
                neighbors.append(P)
        if len(neighbors) >= min_pts:
            assert(point in neighbors)
            neighbors.remove(point)
            return neighbors
        return None
    
    cluster = 0
    label = {}
    for P in points:
        if P in label:
            continue
        neighbors = range_query(points, dist_func, P, eps, min_pts)
        if not neighbors:
            label[P] = NOISE
            continue
        cluster += 1
        label[P] = cluster
        while neighbors:
            all_new = []
            for Q in neighbors:
                if Q in label and label[Q] != NOISE:
                    continue 
                label[Q] = cluster
                new = range_query(points, dist_func, Q, eps, min_pts)
                if new:
                    all_new += new
            neighbors = all_new  
    return label

def test():
    points = ((1,1),(2,1),(1,2),(3,3),(2,2),(2,3),(3,2),
              (10,9),(9,10),(11,10),(11,11),(10,10),(9,11),
              (100,200),(0,200))
    dist_func = lambda P,Q: math.sqrt((P[0] - Q[0]) ** 2 + (P[1] - Q[1]) ** 2)
    eps = 4
    min_pts = 5
    label = dbscan(points, dist_func, eps, min_pts)
    print(label)

if __name__ == '__main__':
    test()
