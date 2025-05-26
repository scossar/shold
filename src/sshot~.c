// a copy of the Pd snapshot~ object

#include "m_pd.h"

static t_class *sshot_class = NULL;

typedef struct _sshot {
  t_object x_obj;
  t_sample x_value;
  t_float x_f;
} t_sshot;

static void *sshot_new(void)
{
  t_sshot *x = (t_sshot *)pd_new(sshot_class);
  x->x_value = 0;
  outlet_new(&x->x_obj, &s_float);
  x->x_f = 0;
  return (void *)x;
}

static t_int *sshot_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  *out = *in;
  return (w+3);
}

static void sshot_dsp(t_sshot *x, t_signal **sp)
{
  dsp_add(sshot_perform, 2, sp[0]->s_vec + (sp[0]->s_length-1),
          &x->x_value);
}

static void sshot_bang(t_sshot *x)
{
  outlet_float(x->x_obj.ob_outlet, x->x_value);
}

static void sshot_set(t_sshot *x, t_floatarg f)
{
  x->x_value = f;
}

void sshot_tilde_setup(void)
{
  sshot_class = class_new(gensym("sshot~"),
                          sshot_new,
                          0,
                          sizeof(t_sshot),
                          CLASS_DEFAULT,
                          0);
  CLASS_MAINSIGNALIN(sshot_class, t_sshot, x_f);
  class_addmethod(sshot_class, (t_method)sshot_dsp,
                  gensym("dsp"), A_CANT, 0);
  class_addmethod(sshot_class, (t_method)sshot_set,
                  gensym("set"), A_DEFFLOAT, 0);
  class_addbang(sshot_class, sshot_bang);
}
