-include ../../common.mk
CHARMC=../../../bin/charmc $(OPTS) 

all: pgm

pgm: pgm.o cst_nl.o
	$(CHARMC) -o pgm pgm.o cst_nl.o -language fem -module netfem 

pgm.o: pgm.C
	$(CHARMC) -c pgm.C

cst_nl.o: cst_nl.C
	$(CHARMC) -c cst_nl.C


test: pgm
	$(call run, ./pgm +vp4 +p2 )

clean:
	rm -f pgm fpgm *.o conv-host charmrun charmrun.exe pgm.exe pgm.pdb pgm.ilk
