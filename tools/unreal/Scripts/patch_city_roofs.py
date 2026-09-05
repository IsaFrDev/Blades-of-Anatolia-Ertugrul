# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtProcMesh.h')
h = rep(h, "// tabiat: kigiz, qoya, po'stloq, barg", "// tabiat: kigiz, qoya, po'stloq, barg\n\tconstexpr float StyleMudRoof = 0.92f, StyleCobble = 0.95f;   // tekis tuproq tom, ko'cha tosh yotqizmasi")
save('ErtProcMesh.h', h)

c = load('ErtWorldBuilder.cpp')
# AddHouse: tom Wall*0.75 alfani jun diapazoniga tushirardi -> tuproq tom; gumbaz oddiy
c = rep(c, "\tM.AddBox(W(E, N, Z + H + 0.15f), FVector(HV + 0.25f, HU + 0.25f, 0.15f) * 100.f, Wall * 0.75f, R);\n\tif (S % 3 == 0) M.AddSphere(W(E, N, Z + H + 0.2f), FMath::Min(HU, HV) * 0.8f, 8, Wall * 0.9f, FVector(1, 1, 0.6f));",
        "\tM.AddBox(W(E, N, Z + H + 0.15f), FVector(HV + 0.25f, HU + 0.25f, 0.15f) * 100.f, ErtCol::Sty(Wall * 0.75f, ErtCol::StyleMudRoof), R);\n\tM.AddBox(W(E, N, Z + H + 0.32f), FVector(HV + 0.28f, HU + 0.28f, 0.06f) * 100.f, ErtCol::Sty(Wall * 0.8f, ErtCol::StyleMudRoof), R);   // tom chekkasi\n\tif (S % 3 == 0) M.AddSphere(W(E, N, Z + H + 0.2f), FMath::Min(HU, HV) * 0.8f, 8, ErtCol::Sty(Wall * 0.9f, ErtCol::StylePlain), FVector(1, 1, 0.6f));")
# Bagras shahri: ko'cha va maydon tosh yotqizmasi, bozor ko'chasi, darvoza yo'llari
c = rep(c, "\t\t// Chorraha favvorasi\n\t\tM.AddCylinder(W(CE(0), CN(28), Z), 3.f, 3.f, 0.8f, 12, Stone, false);",
        '''\t\t// Ko'chalar va markaziy maydon: tosh yotqizma (cobble uslubi), yer ustida 6 sm
\t\t{
\t\t\tconst FLinearColor Cob = ErtCol::Sty(FLinearColor(0.58f, 0.54f, 0.48f), ErtCol::StyleCobble);
\t\t\tM.AddBox(W(CE(0), CN(0), Z + 0.03f), FVector(4200, 4200, 6), Cob);                                  // maydon
\t\t\tfor (int32 g = 0; g < 4; ++g)
\t\t\t{
\t\t\t\tconst float A = g * HALF_PI, L = CityR - 42.f + 14.f;
\t\t\t\tconst float mu = FMath::Cos(A) * (42.f + L * 0.5f), mv = FMath::Sin(A) * (42.f + L * 0.5f);
\t\t\t\tM.AddBox(W(CE(mu), CN(mv), Z + 0.03f), FVector(L * 50.f, 800, 6), ErtCol::Vary(Cob, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));   // 4 ko'cha
\t\t\t}
\t\t\tfor (int32 i = 0; i < 40; ++i)                                                                            // uy oralaridagi tor ko'chalar
\t\t\t{
\t\t\t\tconst float A = i * 2.f * PI / 40 + 0.08f, R0 = 60.f, R1 = CityR - 30.f;
\t\t\t\tif (FMath::Abs(FMath::Sin(A)) < 0.12f || FMath::Abs(FMath::Cos(A)) < 0.12f) continue;
\t\t\t\tM.AddBox(W(CE(FMath::Cos(A) * (R0 + R1) * 0.5f), CN(FMath::Sin(A) * (R0 + R1) * 0.5f), Z + 0.02f), FVector((R1 - R0) * 50.f, 130, 4), ErtCol::Vary(Cob * 0.95f, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));
\t\t\t}
\t\t\tfor (int32 i = 0; i < 6; ++i) M.AddCylinder(W(CE(-30.f + i * 12.f), CN(-36.f), Z + 0.06f), 0.5f, 0.5f, 0.9f, 8, ErtCol::Sty(FLinearColor(0.55f, 0.5f, 0.45f), ErtCol::StyleStone), true);   // maydon chetidagi tosh ustunchalar
\t\t}
\t\t// Chorraha favvorasi
\t\tM.AddCylinder(W(CE(0), CN(28), Z), 3.f, 3.f, 0.8f, 12, Stone, false);''')
# Ochre devorlar: suvoq (oddiy don) - oldingidek; Ochre * 0.9f/0.85f tuslari alfani siljitadi -> Sty plain
c = c.replace("ErtCol::Vary(Ochre * 0.9f, 0.08f, ++S)", "ErtCol::Vary(ErtCol::Sty(Ochre * 0.9f, ErtCol::StyleStone), 0.08f, ++S)")
c = c.replace("ErtCol::Vary(Ochre * 0.85f, 0.08f, ++S)", "ErtCol::Vary(ErtCol::Sty(Ochre * 0.85f, ErtCol::StyleStone), 0.08f, ++S)")
c = c.replace("ErtCol::Vary(Ochre * 0.85f, 0.06f, ++S)", "ErtCol::Vary(ErtCol::Sty(Ochre * 0.85f, ErtCol::StyleStone), 0.06f, ++S)")
c = c.replace("ErtCol::Vary(Ochre * 0.9f, 0.06f, ++S)", "ErtCol::Vary(ErtCol::Sty(Ochre * 0.9f, ErtCol::StyleStone), 0.06f, ++S)")
save('ErtWorldBuilder.cpp', c)

# Material: badan diapazonini bo'lish -> tuproq tom (0.91-0.935), tosh yotqizma (0.935-0.96)
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, "(s < 0.96) ? 9 : (s < 0.99) ? 13 : 10;", "(s < 0.91) ? 9 : (s < 0.935) ? 15 : (s < 0.96) ? 16 : (s < 0.99) ? 13 : 10;")
m = rep(m, "#define HFELT(p) (VN((p) * 35.0) * 0.6 + VN((p) * 140.0) * 0.4)",
        "#define HFELT(p) (VN((p) * 35.0) * 0.6 + VN((p) * 140.0) * 0.4)\n#define HMUD(p) (VN((p) * 5.0) * 0.5 + VN((p) * 22.0) * 0.3 + VN((p) * 80.0) * 0.2 - 0.35 * (1.0 - smoothstep(0.0, 0.03, abs(VN((p) * 4.0) - 0.5))))\n#define CBF(p) (frac((p) / 0.28) - 0.5 + (H2((p) / 0.28) - 0.5) * 0.35)\n#define HCOB(p) (smoothstep(0.46, 0.22, length(CBF(p))) * (0.7 + 0.3 * H2((p) / 0.28)) + 0.08 * VN((p) * 60.0))")
m = rep(m, "(st == 14) ? HFELT(p) : HPLAIN(p))", "(st == 14) ? HFELT(p) : (st == 15) ? HMUD(p) : (st == 16) ? HCOB(p) : HPLAIN(p))")
m = rep(m, "else if (st == 14) { col *= 0.9 + 0.18 * h; }", "else if (st == 14) { col *= 0.9 + 0.18 * h; }\nelse if (st == 15) { col *= 0.72 + 0.4 * saturate(h + 0.2); }\nelse if (st == 16) { col *= 0.5 + 0.55 * h; }")
m = rep(m, "(st == 14) ? 0.01 : 0.02;", "(st == 14) ? 0.01 : (st == 15) ? 0.05 : (st == 16) ? 0.12 : 0.02;")
m = rep(m, "if (s >= 0.86 && s < 0.96) return 0.55 + 0.15 * H;      // badan", "if (s >= 0.86 && s < 0.91) return 0.55 + 0.15 * H;      // badan")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
