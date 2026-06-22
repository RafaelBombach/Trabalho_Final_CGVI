#!/usr/bin/env python3
# Separa o modelo da arvore (sugar maple, exportado com muitos materiais) em
# dois OBJ limpos: tronco (mat0 = casca) e folhas (mat1 = folha), descartando
# o pedaco de chao (mat2) e a grama (mat3) embutidos no modelo. Re-indexa
# v/vt/vn para cada saida.
import os

SRC = r"arvore/source/ImageToStl.com_temp_export.zip/temp_export.zip.obj"
OUTDIR = r"Trabalho_Final_CGVI/data"

# Materiais que queremos manter, e o nome do objeto/arquivo de saida.
WANT = {
    "mat0": ("tree_trunk",  "tree_trunk.obj"),
    "mat1": ("tree_leaves", "tree_leaves.obj"),
}

verts, texs, norms = [], [], []
# faces por material desejado: lista de listas de tokens "v/vt/vn"
faces = {m: [] for m in WANT}

cur_mat = None
with open(SRC, "r") as f:
    for line in f:
        if line.startswith("v "):
            verts.append(line.rstrip("\n"))
        elif line.startswith("vt "):
            texs.append(line.rstrip("\n"))
        elif line.startswith("vn "):
            norms.append(line.rstrip("\n"))
        elif line.startswith("usemtl "):
            cur_mat = line.split()[1]
        elif line.startswith("f "):
            if cur_mat in WANT:
                faces[cur_mat].append(line.split()[1:])

def emit(mat):
    name, fname = WANT[mat]
    used_v, used_t, used_n = {}, {}, {}
    out_faces = []
    for face in faces[mat]:
        toks = []
        for tok in face:
            parts = (tok.split("/") + ["", ""])[:3]
            vi = int(parts[0]) if parts[0] else 0
            ti = int(parts[1]) if parts[1] else 0
            ni = int(parts[2]) if parts[2] else 0
            assert vi > 0 and ti >= 0 and ni >= 0, "indice negativo nao suportado"
            if vi not in used_v: used_v[vi] = len(used_v) + 1
            nt = ""
            if ti:
                if ti not in used_t: used_t[ti] = len(used_t) + 1
            nn = ""
            if ni:
                if ni not in used_n: used_n[ni] = len(used_n) + 1
            t = str(used_v[vi])
            if ti or ni:
                t += "/" + (str(used_t[ti]) if ti else "")
                if ni:
                    t += "/" + str(used_n[ni])
            toks.append(t)
        out_faces.append(toks)

    # Reconstroi listas compactas na ordem dos novos indices.
    inv_v = sorted(used_v, key=lambda k: used_v[k])
    inv_t = sorted(used_t, key=lambda k: used_t[k])
    inv_n = sorted(used_n, key=lambda k: used_n[k])

    path = os.path.join(OUTDIR, fname)
    with open(path, "w") as o:
        o.write("# Gerado por split_tree.py a partir do sugar maple .dae->.obj\n")
        o.write("o " + name + "\n")
        for i in inv_v: o.write(verts[i-1] + "\n")
        for i in inv_t: o.write(texs[i-1] + "\n")
        for i in inv_n: o.write(norms[i-1] + "\n")
        for fc in out_faces:
            o.write("f " + " ".join(fc) + "\n")
    print(f"{fname}: {len(inv_v)} v, {len(inv_t)} vt, {len(inv_n)} vn, {len(out_faces)} faces")

for m in WANT:
    emit(m)
print("OK")
