#ifndef __COMMON_DATASTORE_CONSTRUCT_H__
#define __COMMON_DATASTORE_CONSTRUCT_H__

#include "types.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

class t_construct {
public:
  typedef std::map<std::string, std::string> t_metadata_storage;
  t_metadata_storage _metadata;

  t_construct_id c_id;

  typedef boost::shared_ptr<t_construct> t_ptr;
  typedef boost::shared_ptr<const t_construct> t_const_ptr;

  virtual void metadata(t_construct_id o_id, 
                        std::string key, 
                        std::string value);

};

#endif
