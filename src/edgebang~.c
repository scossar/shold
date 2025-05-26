#include "m_pd.h"

// note: this is more complicated than I was thinking. It's going to need to
// detect amplitude changes.

typedef struct _edgebang {
  t_object x_obj;
  t_float x_envelope;
  int x_attack_ms;
  int x_decay_ms;
  int x_attack_samps;
  int x_decay_samps;

  int x_attack_phase;
  int x_decay_phase;

  t_float x_attack_samps_recip;
  t_float x_decay_samps_recip;

  int x_was_above_threshold;
  int x_is_attacking;
  int x_is_decaying;

  t_float x_attack_threshold;

  t_float x_f; // dummy arg
  t_outlet *x_env_outlet;
  t_outlet *x_bang_outlet;
} t_edgebang;

t_class *edgebang_class = NULL;

static void *edgebang_new(void) {
  t_edgebang *x = (t_edgebang *)pd_new(edgebang_class);

  x->x_envelope = (t_sample)0.0;
  x->x_attack_ms = 20;
  x->x_decay_ms = 100;
  x->x_attack_samps = 0;
  x->x_decay_samps = 0;

  x->x_attack_phase = 0;
  x->x_decay_phase = 0;

  x->x_attack_samps_recip = (t_float)0.0;
  x->x_decay_samps_recip = (t_float)0.0;

  x->x_was_above_threshold = 0;
  x->x_is_attacking = 0;
  x->x_is_decaying = 0;

  x->x_attack_threshold = (t_float)0.5;

  x->x_f = (t_float)220.0;

  x->x_bang_outlet = outlet_new(&x->x_obj, &s_bang);
  x->x_env_outlet = outlet_new(&x->x_obj, &s_signal);

  return (void *)x;
}

static t_int *edgebang_perform(t_int *w) {
  t_edgebang *x = (t_edgebang *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  int n = (int)(w[4]);

  t_sample env = x->x_envelope;
  int attack_samps = x->x_attack_samps;
  int decay_samps = x->x_decay_samps;
  t_float attack_samps_recip = x->x_attack_samps_recip;
  t_float decay_samps_recip = x->x_decay_samps_recip;
  int was_above_threshold = x->x_was_above_threshold;

  int attack_phase = x->x_attack_phase;
  int decay_phase = x->x_decay_phase;
  int is_attacking = x->x_is_attacking;
  int is_decaying = x->x_is_decaying;

  t_float threshold = x->x_attack_threshold;

  while (n--) {
    t_sample current = *in++;
    if (current > env) is_attacking = 1;
    if (is_attacking) {
      env = attack_phase * attack_samps_recip * (env - current) + current;
      attack_phase++;
      if (attack_phase == attack_samps) {
        attack_phase = 0;
        is_attacking = 0;
        is_decaying = 1;
      }
    }

    if (is_decaying) {
      env = decay_phase * decay_samps_recip * env;
      decay_phase++;
      if (decay_phase == decay_samps) {
        decay_phase = 0;
        is_decaying = 0;
      }
    }

    int above_threshold = env > threshold;
    if (above_threshold && !was_above_threshold) {
      outlet_bang(x->x_bang_outlet);
      was_above_threshold = 1;
    }

    *out++ = env;
  }

  x->x_envelope = env;
  x->x_attack_phase = attack_phase;
  x->x_decay_phase = decay_phase;
  x->x_was_above_threshold = was_above_threshold;

  return (w+5);
}

static void edgebang_dsp(t_edgebang *x, t_signal **sp) {
  t_float samps_per_ms = sp[0]->s_sr * (t_float)0.001;
  x->x_attack_samps = (int)(x->x_attack_ms * samps_per_ms);
  x->x_decay_samps = (int)(x->x_decay_ms * samps_per_ms);
  x->x_attack_samps_recip = (t_float)1.0 / (t_float)x->x_attack_samps;
  x->x_decay_samps_recip = (t_float)1.0 / (t_float)x->x_decay_samps;

  dsp_add(edgebang_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_length);
}

static void set_attack_threshold(t_edgebang *x, t_floatarg f) {
  x->x_attack_threshold = f;
}

static void set_attack_ms(t_edgebang *x, t_floatarg f) {
  x->x_attack_ms = (int)f;
}

static void set_decay_ms(t_edgebang *x, t_floatarg f) {
  x->x_decay_ms = (int)f;
}

static void edgebang_free(t_edgebang *x) {
  if (x->x_bang_outlet) outlet_free(x->x_bang_outlet);
  if (x->x_env_outlet) outlet_free(x->x_env_outlet);
}

void edgebang_tilde_setup(void) {
  edgebang_class = class_new(gensym("edgebang~"),
                             (t_newmethod)edgebang_new,
                             (t_method)edgebang_free,
                             sizeof(t_edgebang),
                             CLASS_DEFAULT,
                             0);
  class_addmethod(edgebang_class, (t_method)edgebang_dsp, gensym("dsp"), A_CANT, 0);
  class_addmethod(edgebang_class, (t_method)set_attack_threshold, gensym("threshold"), A_FLOAT, 0);
  class_addmethod(edgebang_class, (t_method)set_attack_ms, gensym("attack_ms"), A_FLOAT, 0);
  class_addmethod(edgebang_class, (t_method)set_decay_ms, gensym("decay_ms"), A_FLOAT, 0);
  CLASS_MAINSIGNALIN(edgebang_class, t_edgebang, x_f);
}
