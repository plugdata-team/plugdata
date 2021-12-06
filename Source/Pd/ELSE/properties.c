#include "m_pd.h"
#include "g_canvas.h"

typedef struct _cnv_objlist{
    const t_pd *obj;
    struct _cnv_objlist *next;
}t_cnv_objlist;

typedef struct _cnv_canvaslist{
    const t_pd              *parent;
    t_cnv_objlist           *obj;
    struct _cnv_canvaslist  *next;
}t_cnv_canvaslist;

static t_cnv_canvaslist *s_canvaslist = 0;

static t_cnv_canvaslist *findCanvas(const t_pd *parent){
    t_cnv_canvaslist*list = s_canvaslist;
    if(!parent  || !list)
        return 0;
    for(list = s_canvaslist; list; list = list->next){
        if(parent == list->parent)
            return list;
    }
    return 0;
}

static t_cnv_objlist *objectsInCanvas(const t_pd *parent){
    t_cnv_canvaslist *list = findCanvas(parent);
    if(list)
        return list->obj;
    return 0;
}

static t_cnv_canvaslist *addCanvas(const t_pd *parent){
    t_cnv_canvaslist *list = findCanvas(parent);
    if(!list){
        list = getbytes(sizeof(*list));
        list->parent = parent;
        list->obj = 0;
        list->next = 0;
        if(s_canvaslist == 0) // new list
            s_canvaslist = list;
        else{ // add to the end of existing list
            t_cnv_canvaslist *dummy = s_canvaslist;
            while(dummy->next)
                dummy = dummy->next;
            dummy->next = list;
        }
    }
    return list;
}

static void removeObjectFromCanvas(const t_pd*parent, const t_pd*obj) {
    t_cnv_canvaslist*p=findCanvas(parent);
    t_cnv_objlist*list=0, *last=0, *next=0;
    if(!p || !obj)return;
    list=p->obj;
    if(!list)
        return;
    
    while(list && obj!=list->obj) {
        last=list;
        list=list->next;
    }
    
    if(!list) /* couldn't find this object */
        return;
    
    next=list->next;
    
    if(last)
        last->next=next;
    else
        p->obj=next;
    
    freebytes(list, sizeof(*list));
    list=0;
}

static void removeObjectFromCanvases(const t_pd*obj) {
    t_cnv_canvaslist*parents=s_canvaslist;
    
    while(parents) {
        removeObjectFromCanvas(parents->parent, obj);
        parents=parents->next;
    }
}

static void addObjectToCanvas(const t_pd *parent, const t_pd *obj){
    t_cnv_canvaslist *p = addCanvas(parent);
    t_cnv_objlist *list = 0;
    t_cnv_objlist *entry = 0;
    if(!p || !obj)
        return;
    list = p->obj;
    if(list && obj == list->obj)
        return;
    while(list && list->next){
        if(obj == list->obj) // obj already in list
            return;
        list = list->next;
    }
    entry = getbytes(sizeof(*entry)); // at end of list not containing obj yet, so add it
    entry->obj = obj;
    entry->next = 0;
    if(list)
        list->next = entry;
    else
        p->obj = entry;
}

///////////////////////
// start of properties_class
///////////////////////

static t_class *properties_class;

typedef struct _properties{
  t_object  x_obj;
  t_outlet *x_properties_bangout;
}t_properties;

t_propertiesfn x_properties_fn = NULL;

static void properties_free(t_properties *x){
  removeObjectFromCanvases((t_pd*)x);
}

static void properties_bang(t_properties *x){
    outlet_bang(x->x_properties_bangout);
}

static void properties_properties(t_gobj *z, t_glist *owner){
  t_cnv_objlist *objs = objectsInCanvas((t_pd*)z);
  if(objs == NULL)
    x_properties_fn(z, owner);
  while(objs){
      t_properties* x = (t_properties*)objs->obj;
      properties_bang(x);
      objs = objs->next;
  }
}

static void *properties_new(t_floatarg f){
    t_properties *x = (t_properties *)pd_new(properties_class);
    t_int subpatch_mode = f != 0;
    t_glist *glist = (t_glist *)canvas_getcurrent();
    t_canvas *canvas = (t_canvas*)glist_getcanvas(glist);
    t_class *class = ((t_gobj*)canvas)->g_pd;
    
    if(!subpatch_mode){
        while(!canvas->gl_env)
            canvas = canvas->gl_owner; // yeah
    }
    t_propertiesfn class_properties_fn = NULL;
    // get properties from the class
         class_properties_fn = class_getpropertiesfn(class);
         if(class_properties_fn != properties_properties)
             x_properties_fn = class_properties_fn;
         // else is NULL
    // set properties
         class_setpropertiesfn(class, properties_properties);
         x->x_properties_bangout = outlet_new((t_object *)x, &s_bang);
    addObjectToCanvas((t_pd*)canvas, (t_pd*)x);
    return(x);
}

void properties_setup(void){
  properties_class = class_new(gensym("properties"), (t_newmethod)properties_new,
            (t_method)properties_free, sizeof(t_properties), CLASS_NOINLET, A_DEFFLOAT, 0);
    x_properties_fn = NULL;
}
