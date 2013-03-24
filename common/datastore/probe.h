#ifndef __COMMON_DATASTORE_PROBE_H__
#define __COMMON_DATASTORE_PROBE_H__

#include "types.h"

#include "datastore/construct.h"
#include "datastore/observation.h"

class t_probe_construct: public t_construct {
public:
  typedef boost::shared_ptr<t_probe_construct> t_ptr;
  typedef boost::shared_ptr<const t_probe_construct> t_const_ptr;
};

struct t_state {
public:
  unsigned char * data;
  size_t * offset_end;
  size_t n_vars;

public:
  t_state();
  ~t_state();

  void read_variable(const void * var,
                     size_t bytes);

  inline size_t data_size() const {
    if(n_vars)
      return offset_end[n_vars - 1];
    else
      return 0;
  }

  inline size_t size() const {
    return sizeof(size_t) * n_vars + data_size();
  }
};

struct t_probe_observation: public t_observation_single {

public:
  typedef boost::shared_ptr<t_probe_observation> t_ptr;
  typedef boost::shared_ptr<const t_probe_observation> t_const_ptr;

  t_state * state;

  t_probe_observation(t_time_interval time,
                      t_construct_id c_id);
  ~t_probe_observation();

  void read_variable(const void * var,
                     size_t bytes);

  virtual size_t size() const;
};

#endif
