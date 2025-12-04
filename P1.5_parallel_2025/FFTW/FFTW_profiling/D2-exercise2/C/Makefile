CC=mpicc
CFLAGS = -O2 -g -Wall -I./ -I$(FFTW_INC) 
LIBS = -L$(FFTW_LIB) -lfftw3_mpi -lfftw3 -lm		

EXE = diffusion.x
SRC = diffusion.c plot_data.c fft_wrapper.c derivative.c 
HEAD = utilities.h
OBJ = $(SRC:.c=.o)


all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $^ $(LIBS) -o $@

%.o : %.c $(HEAD)
	$(CC) $(CFLAGS) -c $<


# Clean generated data/ and frames/, plus common artifacts
flush:
	@rm -f data/concentration_*.dat data/1d_conc_*.dat data/diffusivity_*.dat || true
	@rm -rf frames_2d || true
	@rm -f flame*.svg out.folded perf.data* *.gif *.png || true

clean:
	@rm -f *.o *.x *~ core core.* vgcore.* *.btr  *.dat

distclean: clean flush

