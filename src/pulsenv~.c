#include "m_pd.h"
#include <math.h>

/**
 * notes:
 * - pulse_based_envelope.md
 **/

typedef struct _pulsenv {
  t_object x_obj;
  t_float x_f;
  t_sample x_previous;

  t_float x_threshold;
  int x_in_env;

  int x_attack_ms;
  int x_in_attack;

  int x_release_ms;
  int x_in_release;

  t_float x_target;
  t_float x_attack_coeff;
  t_float x_release_coeff;

  t_sample x_env;

  t_float x_samps_per_ms;
} t_pulsenv;

static t_class *pulsenv_class = NULL;

static void *pulsenv_new(void) {
  t_pulsenv *x = (t_pulsenv *)pd_new(pulsenv_class);

  x->x_threshold = (t_float)0.5;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);

  x->x_previous = (t_sample)0.0;
  x->x_in_env = 0;
  
  x->x_attack_ms = (int)30;
  x->x_in_attack = (int)0;

  x->x_release_ms = (int)100;
  x->x_in_release = (int)0;

  x->x_samps_per_ms = (t_float)0.0;

  x->x_target = (t_float)0.0;
  x->x_attack_coeff = (t_float)0.01;
  x->x_release_coeff = (t_float)0.001;

  x->x_env = (t_sample)0.0;

  x->x_f = 0;

  return (void *)x;
}

static t_int *pulsenv_perform(t_int *w) {
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_pulsenv *x = (t_pulsenv *)(w[3]);
  int n = (int)(w[4]);
  int i;

  t_float samps_per_ms = x->x_samps_per_ms;

  t_sample prev = x->x_previous;
  t_float threshold = x->x_threshold;
  int in_env = x->x_in_env;

  int in_attack = x->x_in_attack;
  int attack_samps = (int)(samps_per_ms * x->x_attack_ms);

  int in_release = x->x_in_release;
  int release_samps = (int)(samps_per_ms * x->x_release_ms);

  t_float target = x->x_target;
  t_sample env = x->x_env;
  t_float attack_coeff = x->x_attack_coeff;
  t_float release_coeff = x->x_release_coeff;

  for (i = 0; i < n; i++, in1++) {
    t_sample next = *in1;
    if (next - prev > threshold) {
      in_env = 1;
      in_attack = 1;
      target = (t_float)1.0;

    } else if (prev - next > threshold) {
      in_env = 0;
      in_release = 1;
      target = (t_float)0.0;
    }

    t_float coeff = (target > env) ? attack_coeff : release_coeff;

    env += coeff * (target - env);

    *out++ = env;
    prev = next;
  }

  x->x_target = target;
  x->x_env = env;

  x->x_previous = prev;
  x->x_in_env = in_env;

  x->x_in_attack = in_attack;

  x->x_in_release = in_release;

  return (w+5);
}

static void pulsenv_dsp(t_pulsenv *x, t_signal **sp) {
  x->x_samps_per_ms = sp[0]->s_sr * (t_float)0.001;
  dsp_add(pulsenv_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec,
          x, (t_int)sp[0]->s_length);
}

static void set_threshold(t_pulsenv *x, t_floatarg f) {
  x->x_threshold = f;
}

static void set_attack_ms(t_pulsenv *x, t_floatarg f) {
  x->x_attack_ms = (f > 0) ? f : 0;
}

static void set_release_ms(t_pulsenv *x, t_floatarg f) {
  x->x_release_ms = (f > 0) ? f : 0;
}

void pulsenv_tilde_setup(void) {
  pulsenv_class = class_new(gensym("pulsenv~"),
                          (t_newmethod)pulsenv_new,
                          0,
                          sizeof(t_pulsenv),
                          CLASS_DEFAULT, 0);
  class_addmethod(pulsenv_class, (t_method)pulsenv_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(pulsenv_class, (t_method)set_threshold, gensym("threshold"), A_FLOAT, 0);
  class_addmethod(pulsenv_class, (t_method)set_attack_ms, gensym("attack"), A_FLOAT, 0);
  class_addmethod(pulsenv_class, (t_method)set_release_ms, gensym("release"), A_FLOAT, 0);
  CLASS_MAINSIGNALIN(pulsenv_class, t_pulsenv, x_f);
}
