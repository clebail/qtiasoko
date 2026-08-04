import re, os, glob, sys
R="/Users/corentin/perso/qtiasoko"
re_m=re.compile(r'^\[mouv\] joueur \((\d+),(\d+)\)->\((\d+),(\d+)\)(?: POUSSE caisse ->\((\d+),(\d+)\))?')
re_lan=re.compile(r'^\[macro\] LANCEE caisse \((\d+),(\d+)\) -> but \((\d+),(\d+)\) : (\d+) poussees')

def charge(p):
    ls=[l.rstrip('\n') for l in open(p) if l.strip()!='']
    L=max(len(l) for l in ls)
    return [list(l.ljust(L)) for l in ls]

def parties(journal):
    """rend la liste des parties ; chaque partie = liste de (p1,p2,p3,idMacro)"""
    out=[]; cur=None; mid=None; nm=0
    for l in open(journal, errors='replace'):
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
    """rend (gagnee, liste des poussees) ; chaque poussee = (idCaisse, idMacro)"""
    g=charge(f"{R}/level{niv:04d}.xsb")
    lib=lambda x,y: g[y][x] in ' .'
    cai=lambda x,y: g[y][x] in '$*'
    def videJ(x,y): g[y][x]='.' if g[y][x]=='+' else ' '
    def poseJ(x,y): g[y][x]='+' if g[y][x]=='.' else '@'
    def videC(x,y): g[y][x]='.' if g[y][x]=='*' else ' '
    def poseC(x,y): g[y][x]='*' if g[y][x]=='.' else '$'
    j=[(x,y) for y,r in enumerate(g) for x,c in enumerate(r) if c in '@+'][0]
    # identite des caisses : id stable, suivi de case en case
    ident={}; n=0
    for y,r in enumerate(g):
        for x,c in enumerate(r):
            if c in '$*': ident[(x,y)]=n; n+=1
    pouss=[]
    for (p1,p2,p3,mid) in coups:
        if j!=p1: return (False,[])
        if p3 is not None:
            if not cai(*p2) or not lib(*p3): return (False,[])
            videC(*p2); poseC(*p3)
            ident[p3]=ident.pop(p2)
            pouss.append((ident[p3], mid))
        else:
            if not lib(*p2): return (False,[])
        videJ(*j); poseJ(*p2); j=p2
    gagne=all(c!='$' for r in g for c in r)
    return (gagne,pouss)

print(f"{'niv':>4} {'pouss':>6} {'macro':>6} {'chois':>6} | {'T.livr':>6} {'T.man':>6} {'TOTAL':>6} | {'cais.man':>8} {'repris':>6}")
print("-"*80)
tot=[]
for f in sorted(glob.glob(f"{R}/hybride_niveau_*.txt")):
    b=os.path.basename(f)
    if '_intentions' in b or '_manques' in b or 'ordre_calcule' in b: continue
    m=re.search(r'hybride_niveau_(\d+)\.txt', b)
    if not m: continue
    niv=int(m.group(1))
    if not os.path.exists(f"{R}/level{niv:04d}.xsb"): continue
    gagnee=None
    for p in parties(f):
        ok,pouss=rejoue(niv,p)
        if ok and pouss: gagnee=pouss
    if gagnee is None: continue
    # segmentation en TACHES
    taches=[]; 
    for (cid,mid) in gagnee:
        if mid is not None:
            if not taches or taches[-1][0]!='M' or taches[-1][2]!=mid: taches.append(['M',cid,mid,0])
        else:
            if not taches or taches[-1][0]!='H' or taches[-1][1]!=cid: taches.append(['H',cid,None,0])
        taches[-1][3]+=1
    livr=[t for t in taches if t[0]=='M']; man=[t for t in taches if t[0]=='H']
    caisses={}
    for t in man: caisses[t[1]]=caisses.get(t[1],0)+1
    repris=sum(1 for c,k in caisses.items() if k>1)
    nmac=sum(1 for (c,mid) in gagnee if mid is not None)
    print(f"{niv:>4} {len(gagnee):>6} {nmac:>6} {len(gagnee)-nmac:>6} | "
          f"{len(livr):>6} {len(man):>6} {len(taches):>6} | {len(caisses):>8} {repris:>6}")
    tot.append((niv,len(gagnee),len(livr),len(man),len(taches),len(caisses),repris))
if tot:
    import statistics as st
    print("-"*80)
    print(f"{'med':>4} {'':>6} {'':>6} {'':>6} | {st.median(t[2] for t in tot):>6.0f} "
          f"{st.median(t[3] for t in tot):>6.0f} {st.median(t[4] for t in tot):>6.0f} | "
          f"{st.median(t[5] for t in tot):>8.0f} {st.median(t[6] for t in tot):>6.0f}")
