
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <assert.h>

#include <sys/mman.h>
#include <stdint.h>
#include <sched.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 1024
#define MAGIC_NUMBER_ONE 20.8
#define MEASURE_FOR 1000
#define SAMPLE_RATE 44100


#define FIXED_POINT  16

#include "kiss_fft.h"
#include "kiss_fftr.h"


int is_speech = 0;

struct stats
{
  int min_e;
  int min_f;
  int min_sf;
};

long long
get_frame_energy (int16_t frame[], size_t len)
{

  long int i;
  long long sum = 0;

  for (i = 0; i <= len; i++)
    {

      /* FIXME: not sure if this is supposed to be abs */

      /* FIXME: no overflow check.. :(  */
      sum = abs (frame[i]) + sum;

    }

  assert (sum >= 0);
  return sum;
}

unsigned long long
get_sample_intensity (int16_t sample[], size_t len)
{

  long int intensity = 0;
  unsigned long long sum_squared = 0;

  for (int i = 0; i <= len; i++)
    {
      int sample_squared = sample[i] * sample[i];
      sum_squared = sum_squared + sample_squared;
    }

  intensity =
    MAGIC_NUMBER_ONE * log10 (sqrt (sum_squared / (float) SAMPLE_RATE));

  return (int) intensity;

}

/* Lots of playing around here */
int
get_dom_freq_component (int16_t sample[], size_t len)
{
    int i,size = 12;
    int isinverse = 1;
    float buf[size];
    float array[] = {0.1, 0.6, 0.1, 0.4, 0.5, 0, 0.8, 0.7, 0.8, 0.6, 0.1,0};

    kiss_fft_cpx out_cpx[size], out[size], cpx_buf[size];

    kiss_fftr_cfg fft = kiss_fftr_alloc(size*2 ,0 ,0,0);
    kiss_fftr_cfg ifft = kiss_fftr_alloc(size*2,isinverse,0,0);

    kiss_fft_scalar zero;

    memset(&zero,0,sizeof(zero));

    for (i = 0; i <= size; i++) {
      cpx_buf[i].r = array[i];
      cpx_buf[i].i = zero;
    }

    kiss_fftr(fft,(kiss_fft_scalar*)cpx_buf, out_cpx);
    kiss_fftri(ifft,out_cpx,(kiss_fft_scalar*)out );

    printf("Input:    \tOutput:\n");
    for(i=0;i<size;i++)
    {
	buf[i] = (out[i].r)/(size*2);
	printf("%f\t%f\n",array[i],buf[i]);
    }

    kiss_fft_cleanup();
    free(fft);
    free(ifft);

}

int
arithmatic_mean (int16_t frame[], size_t len)
{
  return 1;
}

int
geometric_mean (int16_t frame[], size_t len)
{

  long int prod = 1;

  for (int i = 0; i <= len; i++)
    {
      prod *= frame[i];
    }

  return pow (prod, 1.0 / len);
}

int
arithmetic_mean (int16_t frame[], size_t len)
{
  return 1;
}


/* This is the main loop */
int
compute_vad (int16_t frame[], int bytes_to_read, int frame_count,
	     struct stats *s)
{

  /* Compute the frame energy */
  long long energy = get_frame_energy (frame, bytes_to_read);
  printf ("energy: %d\n", energy);
  assert (energy >= 0);

  /* FIXME: compute the most dominant freq component */
  int most_dominant_freq_component =
    get_dom_freq_component (frame, bytes_to_read);

  printf ("dom_freq_comp: %d\n", most_dominant_freq_component);
  assert (most_dominant_freq_component >= 0);

  /* FIXME: */
  int spectral_flatness = INT_MAX;
  printf ("spectral flatness %d\n", spectral_flatness);
  assert (most_dominant_freq_component >= 0);

  /* FIXME: Supposing that some of the first 30 frames are silence */
  if (frame_count <= 30)
    {
      /* find the minimum value for
         E ( Min _ E )
         F ( Min _ F )
         SFM ( Min _ SF ) */


      printf ("ENERGY @ FRAME: %d -  %d\n", frame_count, energy);

      if (energy <= s->min_e)
	{
	  s->min_e = energy;
	  printf ("NEW ENERGY MIN:          %d\n", s->min_e);
	}

      if (most_dominant_freq_component < s->min_f)
	{
	  printf ("NEW DOM FREQ MIN:        %d\n", s->min_f);
	  s->min_f = most_dominant_freq_component;
	}

      if (spectral_flatness < s->min_sf)
	{
	  printf ("NEW SPEC FLAT MIN:       %d\n", s->min_sf);
	  s->min_sf = spectral_flatness;
	}

    }

  /* Print some stats out here */
  if (frame_count == 30)
    {
      printf ("ENERGY MIN AT 30 FRAMES:    %d\n", s->min_e);
      printf ("DOM FREQ MIN AT 30 FRAMES : %d\n", s->min_f);
      printf ("SPEC FLAT MIN AT 30 FRAMES: %d\n", s->min_sf);
    }


  /* Apply a FFT on each speech frame */
  if (is_speech)
    {

      /* FIXME: Find F(i) = arg max(S(k)) as the most dominant frequency component. */

      /* FIXME: Compute the abstract value of Spectral Flatness Measure ( SFM (i )) . */


    }

  return 1;
}

int
main (int argc, char *argv[])
{

  /* The sample type to use */
  static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = SAMPLE_RATE,
    .channels = 1
  };

  pa_simple *s = NULL;
  int ret = 1;
  int error;
  int loop_counter = 0;
  int frame_count = 0;

  struct stats *silence_stats;

  silence_stats = malloc (sizeof (struct stats));

  silence_stats->min_e = INT_MAX;
  silence_stats->min_f = INT_MAX;
  silence_stats->min_sf = INT_MAX;

  ssize_t bytes_to_read = 0;

  /* Bytes to read for 10 ms */
  bytes_to_read = pa_usec_to_bytes ((pa_usec_t) 10 * 1000, &ss);

  printf ("Bytes to read: %d!\n", (int) bytes_to_read);

  /* Create the recording stream */
  if (!
      (s =
       pa_simple_new (NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss,
		      NULL, NULL, &error)))
    {
      fprintf (stderr, __FILE__ ": pa_simple_new() failed: %s\n",
	       pa_strerror (error));
      goto finish;
    }

  for (loop_counter = 1; loop_counter <= MEASURE_FOR; loop_counter++)
    {

      int16_t buf[bytes_to_read];

      /* Record some data ... */
      if (pa_simple_read (s, buf, sizeof (buf), &error) < 0)
	{
	  fprintf (stderr, __FILE__ ": pa_simple_read() failed: %s\n",
		   pa_strerror (error));
	  goto finish;
	}

      compute_vad (buf, bytes_to_read, loop_counter, silence_stats);

      /*        printf("%d\n", loop_counter); */
      fflush (stdout);
    }

  ret = 0;

finish:

  if (s)
    pa_simple_free (s);

  return ret;
}
