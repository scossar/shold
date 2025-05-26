#include "m_pd.h"
#include <math.h>

/**
 * notes:
 * - pulse_based_envelope.md
 **/

typedef struct _penv {
  t_object x_obj;
  t_float x_f;
  t_sample x_previous;

  t_float x_threshold;
  int x_in_env;

  int x_attack_ms;
  int x_attack_phase;
  int x_in_attack;
  t_float x_attack_recip;

  int x_release_ms;
  int x_release_phase;
  int x_in_release;
  int x_release_recip;

  t_float x_attack_curve;
  t_float x_release_curve;

  t_float x_samps_per_ms;
} t_penv;

static t_class *penv_class = NULL;

static void *penv_new(void) {
  t_penv *x = (t_penv *)pd_new(penv_class);

  x->x_threshold = (t_float)0.5;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);

  x->x_previous = (t_sample)0.0;
  x->x_in_env = 0;
  
  x->x_attack_ms = (int)30;
  x->x_attack_phase = (int)0;
  x->x_in_attack = (int)0;
  x->x_attack_recip = (t_float)0.0;

  x->x_release_ms = (int)100;
  x->x_release_phase = (int)0;
  x->x_in_release = (int)0;
  x->x_release_recip = (t_float)0.0;

  x->x_attack_curve = (t_float)2.0;
  x->x_release_curve = (t_float)0.5;

  x->x_samps_per_ms = (t_float)0.0;

  x->x_f = 0;

  return (void *)x;
}

static t_int *penv_perform(t_int *w) {
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_penv *x = (t_penv *)(w[3]);
  int n = (int)(w[4]);

  t_float samps_per_ms = x->x_samps_per_ms;

  t_sample prev = x->x_previous;
  t_float threshold = x->x_threshold;
  int in_env = x->x_in_env;
  t_sample env = (t_sample)0.0;

  int attack_phase = x->x_attack_phase;
  int in_attack = x->x_in_attack;
  int attack_samps = (int)(samps_per_ms * x->x_attack_ms);
  t_float attack_recip = (t_float)((t_float)1.0 / attack_samps);

  int release_phase = x->x_release_phase;
  int in_release = x->x_in_release;
  int release_samps = (int)(samps_per_ms * x->x_release_ms);
  t_float release_recip = (t_float)((t_float)1.0 / release_samps);

  t_float attack_curve = x->x_attack_curve;
  t_float release_curve = x->x_release_curve;

  while (n--) {
    t_sample next = *in1++;
    if ((next - prev > threshold) && !in_release) {
      in_env = 1;
      in_attack = 1;
    } else if ((prev - next > threshold) && !in_attack) {
      in_env = 0;
      in_release = 1;
    }

    if (in_env) {
      if (in_attack) {
        // todo: use a 1 pole envelope instead of the pow function
        t_float linear_pos = (t_float)(attack_phase * attack_recip);
        env = (t_sample)pow(linear_pos, attack_curve);
        attack_phase++;
        if (attack_phase >= attack_samps) {
          attack_phase = 0;
          in_attack = 0;
        }
      } else {
        env = (t_sample)1.0;
      }
    }

    if (in_release) {
      t_float linear_pos = (t_float)((release_samps - release_phase) * release_recip);
      env = (t_sample)pow(linear_pos, release_curve);
      release_phase++;
      if (release_phase >= release_samps) {
        release_phase = 0;
        in_release = 0;
      }
    }

    *out++ = env;
    prev = next;
  }
  x->x_previous = prev;
  x->x_in_env = in_env;

  x->x_attack_phase = attack_phase;
  x->x_in_attack = in_attack;

  x->x_release_phase = release_phase;
  x->x_in_release = in_release;

  return (w+5);
}

static void penv_dsp(t_penv *x, t_signal **sp) {
  x->x_samps_per_ms = sp[0]->s_sr * (t_float)0.001;
  dsp_add(penv_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec,
          x, (t_int)sp[0]->s_length);
}

static void set_threshold(t_penv *x, t_floatarg f) {
  x->x_threshold = f;
}

static void set_attack_ms(t_penv *x, t_floatarg f) {
  x->x_attack_ms = (f > 0) ? f : 0;
}

static void set_attack_curve(t_penv *x, t_floatarg f) {
  x->x_attack_curve = (f > 0) ? f : 0;
}

static void set_release_ms(t_penv *x, t_floatarg f) {
  x->x_release_ms = (f > 0) ? f : 0;
}

static void set_release_curve(t_penv *x, t_floatarg f) {
  x->x_release_curve = (f > 0) ? f : 0;
}

void penv_tilde_setup(void) {
  penv_class = class_new(gensym("penv~"),
                          (t_newmethod)penv_new,
                          0,
                          sizeof(t_penv),
                          CLASS_DEFAULT, 0);
  class_addmethod(penv_class, (t_method)penv_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(penv_class, (t_method)set_threshold, gensym("threshold"), A_FLOAT, 0);
  class_addmethod(penv_class, (t_method)set_attack_ms, gensym("attack"), A_FLOAT, 0);
  class_addmethod(penv_class, (t_method)set_release_ms, gensym("release"), A_FLOAT, 0);
  class_addmethod(penv_class, (t_method)set_attack_curve, gensym("attack_curve"), A_FLOAT, 0);
  class_addmethod(penv_class, (t_method)set_release_curve, gensym("release_curve"), A_FLOAT, 0);
  CLASS_MAINSIGNALIN(penv_class, t_penv, x_f);
}
