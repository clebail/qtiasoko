import re, os, glob
import statistics as st
R="/Users/corentin/perso/qtiasoko"
re_m=re.compile(r'^\[mouv\] joueur \((\d+),(\d+)\)->\((\d+),(\d+)\)(?: POUSSE caisse ->\((\d+),(\d+)\))?')
re_lan=re.compile(r'^\[macro\] LANCEE caisse')
res={1,2,3,4,5,6,7,8,9,10,11,17,21,32}

def charge(p):
    ls=[l.rstrip('\n') for l in open(p) if l.strip()!='']
    L=max(len(l) for l in ls); return [list(l.ljust(L)) for l in ls]

def parties(j):
    out=[]; cur=None; mid=None; nm=0
    for l in open(j, errors='replace'):
        l=l.rstrip('\n')
        if l.startswith('=== niveau'):
            if cur is not None: out.append(cur)
            cur=[]; mid=None; continue
        if cur is None: continue
        if l.startswith('[undo]'):
            if cur: cur.pop()
            continue
        if re_lan.match(l): nm+=1; mid=nm; continue
        if l.startswith('[macro] TERMINEE'): mid=None; continue
        m=re_m.match(l)
        if m:
            g=m.groups()
            cur.append(((int(g[0]),int(g[1])),(int(g[2]),int(g[3])),
                        None if g[4] is None else (int(g[4]),int(g[5])), mid))
    if cur is not None: out.append(cur)
    return out

def rejoue(niv, coups):
    g=charge(f"{R}/level{niv:04d}.xsb")
    lib=lambda x,y: g[y][x] in ' .'; cai=lambda x,y: g[y][x] in '$*'
    def vJ(x,y): g[y][x]='.' if g[y][x]=='+' else ' '
    def pJ(x,y): g[y][x]='+' if g[y][x]=='.' else '@'
    def vC(x,y): g[y][x]='.' if g[y][x]=='*' else ' '
    def pC(x,y): g[y][x]='*' if g[y][x]=='.' else '$'
    j=[(x,y) for y,r in enumerate(g) for x,c in enumerate(r) if c in '@+'][0]
    ident={}; n=0
    for y,r in enumerate(g):
        for x,c in enumerate(r):
            if c in '$*': ident[(x,y)]=n; n+=1
    pouss=[]
    for (p1,p2,p3,mid) in coups:
        if j!=p1: return (False,[],None)
        if p3 is not None:
            if not cai(*p2) or not lib(*p3): return (False,[],None)
            vC(*p2); pC(*p3); ident[p3]=ident.pop(p2)
            pouss.append((ident[p3], mid, p2, p3))
        elif not lib(*p2): return (False,[],None)
        vJ(*j); pJ(*p2); j=p2
    return (all(c!='$' for r in g for c in r), pouss, charge(f"{R}/level{niv:04d}.xsb"))

print(f"{'niv':>4} {'':>1} {'garages':>7} {'cases':>6} {'top':>4} | {'murs voisins':>12} | {'sur but':>7}")
print("-"*62)
glob_mur=[]; glob_base=[]; lignes=[]
for f in sorted(glob.glob(f"{R}/hybride_niveau_*.txt")):
    b=os.path.basename(f)
    if any(k in b for k in ('_intentions','_manques','ordre_calcule')): continue
    m=re.search(r'hybride_niveau_(\d+)\.txt', b)
    if not m or not os.path.exists(f"{R}/level{int(m.group(1)):04d}.xsb"): continue
    niv=int(m.group(1)); best=None
    for p in parties(f):
        ok,pouss,plateau=rejoue(niv,p)
        if ok and pouss: best=(pouss,plateau)
    if not best: continue
    pouss,pl=best
    H=len(pl); L=len(pl[0])
    murs=lambda x,y: sum(1 for dx,dy in((0,-1),(1,0),(0,1),(-1,0))
                         if not(0<=x+dx<L and 0<=y+dy<H) or pl[y+dy][x+dx]=='#')
    # segmentation en taches, avec source/destination
    taches=[]
    for (cid,mid,src,dst) in pouss:
        if mid is not None:
            if not taches or taches[-1]['t']!='M' or taches[-1]['id']!=mid:
                taches.append({'t':'M','c':cid,'id':mid,'src':src,'dst':dst})
        else:
            if not taches or taches[-1]['t']!='H' or taches[-1]['c']!=cid:
                taches.append({'t':'H','c':cid,'id':None,'src':src,'dst':dst})
        taches[-1]['dst']=dst
    # un GARAGE = une manoeuvre dont la caisse rebouge PLUS TARD
    gar=[]
    for i,t in enumerate(taches):
        if t['t']!='H': continue
        if any(u['c']==t['c'] for u in taches[i+1:]): gar.append(t['dst'])
    if not gar: continue
    cases={}
    for c in gar: cases[c]=cases.get(c,0)+1
    top=max(cases.values())
    mv=[murs(*c) for c in gar]; glob_mur+=mv
    surbut=sum(1 for c in gar if pl[c[1]][c[0]] in '.*+')
    base=[murs(x,y) for y in range(H) for x in range(L) if pl[y][x] in ' .$*@+']
    glob_base+=base
    print(f"{niv:>4} {'R' if niv in res else '.':>1} {len(gar):>7} {len(cases):>6} {top:>4} | "
          f"{st.mean(mv):>12.2f} | {100*surbut/len(gar):>6.0f}%")
    lignes.append((niv,len(gar),len(cases)))
print("-"*62)
print(f"murs voisins — cases de GARAGE : {st.mean(glob_mur):.2f}   "
      f"toutes cases libres : {st.mean(glob_base):.2f}")
print(f"garages/cases distinctes — total {sum(l[1] for l in lignes)} garages sur "
      f"{sum(l[2] for l in lignes)} cases distinctes")
