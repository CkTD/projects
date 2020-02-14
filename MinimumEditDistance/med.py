D={}  #   'a_b' ->  (dist, path)
def minimum_edit_distance(a, b):
    global D
    if '%s_%s' % (a,b) in D:
        return D['%s_%s' % (a,b)][0]

    if a==b:
        D['%s_%s' % (a, b)] = (0,' '*len(a))
    elif not a:
        D['%s_%s' % (a, b)] = (len(b), 'i'*len(b))
    elif not b:
        D['%s_%s' % (a, b)] = (len(a), 'd'*len(a))
    else:
        d_m = minimum_edit_distance(a[:-1], b) + 1
        i_m = minimum_edit_distance(a, b[:-1]) + 1
        s_m = minimum_edit_distance(a[:-1], b[:-1]) + (0 if a[-1] == b[-1] else 1)
        sum_m = min(d_m, i_m, s_m)
        if sum_m == d_m:
            D['%s_%s' % (a, b)] = (sum_m, D['%s_%s' % (a[:-1],b)][1] + 'd')
        elif sum_m == i_m:
            D['%s_%s' % (a, b)] = (sum_m, D['%s_%s' % (a,b[:-1])][1] + 'i')
        else:
            D['%s_%s' % (a, b)] = (sum_m, D['%s_%s' % (a[:-1],b[:-1])][1] + ('k' if a[-1] == b[-1] else 's'))
            
    return D['%s_%s' % (a,b)][0]

a="intention" # source
b="execution" # target

minimum_edit_distance(a,b)
print("%s -> %s" % (a,b))
print(D['%s_%s' % (a,b)])
