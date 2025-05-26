#include "m_pd.h"
#include <math.h>

/**
 * a one-pole filter exponential envelope
 *
 * notes:
 * - pulse_based_envelope.md
 **/

typedef enum {
  ENV_IDLE,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE
} t_env_state;

typedef struct _pulsenv {
  t_object x_obj;

  t_float x_target;
  t_sample x_env;

  t_float x_attack_coeff;
  t_float x_decay_coeff;
  t_float x_sustain_level;
  t_float x_release_coeff;

  t_float x_previous;
  t_float x_threshold;

  t_env_state x_state;

  t_float x_f;
} t_pulsenv;

static t_class *pulsenv_class = NULL;

static void *pulsenv_new(void) {
  t_pulsenv *x = (t_pulsenv *)pd_new(pulsenv_class);

  x->x_threshold = (t_float)0.5;
  x->x_previous = (t_float)0.0;
  x->x_target = (t_float)0.0;
  x->x_attack_coeff = (t_float)0.01;
  x->x_decay_coeff = (t_float)0.01;
  x->x_sustain_level = (t_float)0.66;
  x->x_release_coeff = (t_float)0.001;

  x->x_env = (t_sample)0.0;

  x->x_state = ENV_IDLE;

  x->x_f = 0;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  outlet_new(&x->x_obj, &s_signal);

  return (void *)x;
}

static t_int *pulsenv_perform(t_int *w) {
  t_sample *in1 = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_pulsenv *x = (t_pulsenv *)(w[3]);
  int n = (int)(w[4]);

  t_float threshold = x->x_threshold;
  t_float prev = x->x_previous;
  t_float target = x->x_target;
  t_sample env = x->x_env;
  t_env_state state = x->x_state;

  t_float attack_coeff = x->x_attack_coeff;
  t_float decay_coeff = x->x_decay_coeff;
  t_float release_coeff = x->x_release_coeff;
  t_float sustain_level = x->x_sustain_level;

  t_float close_threshold = (t_float)0.01;

  while (n--) {
    t_sample next = *in1++;
    t_float coeff;

    if (next - prev > threshold) {
      state = ENV_ATTACK;
      target = (t_float)1.0;
    } else if (prev - next > threshold) {
      state = ENV_RELEASE;
      target = (t_float)0.0;
    }

    if (state == ENV_ATTACK && env > ((t_float)1.0 - close_threshold)) {
      state = ENV_DECAY;
      target = sustain_level;
    } else if (state == ENV_DECAY && fabsf(env - sustain_level) < close_threshold) {
      state = ENV_SUSTAIN;
    }

    switch (state) {
      case ENV_ATTACK:
      coeff = attack_coeff;
      break;
      case ENV_DECAY:
      coeff = decay_coeff;
      break;
      case ENV_SUSTAIN:
      coeff = decay_coeff; // for small adjustments
      break;
      case ENV_RELEASE:
      default:
      coeff = release_coeff;
      break;
    }

    env += coeff * (target - env);

    *out++ = env;
    prev = next;
  }

  x->x_state = state;
  x->x_target = target;
  x->x_env = env;
  x->x_previous = prev;

  return (w+5);
}

static void pulsenv_dsp(t_pulsenv *x, t_signal **sp) {
  dsp_add(pulsenv_perform, 4,
          sp[0]->s_vec, sp[1]->s_vec,
          x, (t_int)sp[0]->s_length);
}

static void set_threshold(t_pulsenv *x, t_floatarg f) {
  x->x_threshold = f;
}

// attack in seconds
static void set_attack_coefficient(t_pulsenv *x, t_floatarg f) {
  t_float sr = sys_getsr();
  t_float coeff = (t_float)1.0 - expf((t_float)-1.0 / (sr * f));
  x->x_attack_coeff = coeff;
}

// release in seconds
static void set_release_coefficient(t_pulsenv *x, t_floatarg f) {
  t_float sr = sys_getsr();
  t_float coeff = (t_float)1.0 - expf((t_float)-1.0 / (sr * f));
  x->x_release_coeff = coeff;
}

void pulsenv_tilde_setup(void) {
  pulsenv_class = class_new(gensym("pulsenv~"),
                          (t_newmethod)pulsenv_new,
                          0,
                          sizeof(t_pulsenv),
                          CLASS_DEFAULT, 0);
  class_addmethod(pulsenv_class, (t_method)pulsenv_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(pulsenv_class, (t_method)set_threshold, gensym("threshold"), A_FLOAT, 0);
  class_addmethod(pulsenv_class, (t_method)set_attack_coefficient, gensym("attack"), A_FLOAT, 0);
  class_addmethod(pulsenv_class, (t_method)set_release_coefficient, gensym("release"), A_FLOAT, 0);
  CLASS_MAINSIGNALIN(pulsenv_class, t_pulsenv, x_f);
}
