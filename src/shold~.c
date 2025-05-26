// a copy of the Pd samphold~ object

#include "m_pd.h"

typedef struct _shold {
  t_object x_obj;
  t_float x_f;
  t_sample x_lastin;
  t_sample x_lastout;

  int x_sample_on_high;
  t_float x_threshold;
} t_shold;

// static t_class?
t_class *shold_class = NULL;

static void *shold_new(void) {
  t_shold *x = (t_shold *)pd_new(shold_class);

  x->x_sample_on_high = (int)0;
  x->x_threshold = (t_float)0.5;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);

  x->x_lastin = 0;
  x->x_lastout = 0;
  x->x_f = 0;

  return (void *)x;
}

static t_int *shold_perform(t_int *w) {
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *in2 = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_shold *x = (t_shold *)(w[4]);
  int n = (int)(w[5]);
  int i;
  t_sample lastin = x->x_lastin;
  t_sample lastout = x->x_lastout;
  // treated as a boolean
  int sample_on_high = x->x_sample_on_high;
  t_float threshold = x->x_threshold;

  for (i = 0; i < n; i++, in1++) {
    t_sample next = *in2++;
    if (sample_on_high) {
      if (next - lastin > threshold) lastout = *in1;
    } else {
      if (lastin - next > threshold) lastout = *in1;
    }
    *out++ = lastout;
    lastin = next;
  }
  x->x_lastin = lastin;
  x->x_lastout = lastout;
  return (w+6);
}

static void shold_dsp(t_shold *x, t_signal **sp) {
  dsp_add(shold_perform, 5,
          sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
          x, (t_int)sp[0]->s_length);
}

static void shold_reset(t_shold *x, t_symbol *s, int argc, t_atom *argv) {
  x->x_lastin = ((argc > 0 && (argv[0].a_type == A_FLOAT)) ?
                 argv[0].a_w.w_float : 1e20);
}

static void shold_set(t_shold *x, t_float f) {
  x->x_lastout = f;
}

static void sample_on_high(t_shold *x, t_floatarg f) {
  f = (f > 0) ? (int)1 : (int)0;
  x->x_sample_on_high = f;
}

static void set_threshold(t_shold *x, t_floatarg f) {
  x->x_threshold = f;
}

void shold_tilde_setup(void) {
  shold_class = class_new(gensym("shold~"),
                          (t_newmethod)shold_new,
                          0,
                          sizeof(t_shold),
                          CLASS_DEFAULT, 0);
  CLASS_MAINSIGNALIN(shold_class, t_shold, x_f);
  class_addmethod(shold_class, (t_method)shold_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(shold_class, (t_method)shold_reset, gensym("reset"), A_GIMME, 0);
  class_addmethod(shold_class, (t_method)shold_set, gensym("set"), A_FLOAT, 0);
  class_addmethod(shold_class, (t_method)set_threshold, gensym("threshold"), A_FLOAT, 0);
  class_addmethod(shold_class, (t_method)sample_on_high, gensym("sample_on_high"), A_FLOAT, 0);
}
