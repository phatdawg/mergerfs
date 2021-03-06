/*
  Copyright (c) 2016, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <errno.h>

#include <limits>
#include <string>
#include <vector>

#include "fs.hpp"
#include "fs_path.hpp"
#include "policy.hpp"

using std::string;
using std::vector;
using std::size_t;
using mergerfs::Category;

static
int
_lfs_create(const vector<string>  &basepaths,
            const size_t           minfreespace,
            vector<const string*> &paths)
{
  string fullpath;
  size_t lfs;
  const string *lfsbasepath;

  lfs = std::numeric_limits<size_t>::max();
  lfsbasepath = NULL;
  for(size_t i = 0, ei = basepaths.size(); i != ei; i++)
    {
      bool readonly;
      size_t spaceavail;
      size_t _spaceused;
      const string *basepath = &basepaths[i];

      if(!fs::info(*basepath,readonly,spaceavail,_spaceused))
        continue;
      if(readonly)
        continue;
      if(spaceavail < minfreespace)
        continue;
      if(spaceavail > lfs)
        continue;

      lfs = spaceavail;
      lfsbasepath = basepath;
    }

  if(lfsbasepath == NULL)
    return POLICY_FAIL_ENOENT;

  paths.push_back(lfsbasepath);

  return POLICY_SUCCESS;
}

static
int
_lfs_other(const vector<string>  &basepaths,
           const char            *fusepath,
           vector<const string*> &paths)
{
  string fullpath;
  size_t lfs;
  const string *lfsbasepath;

  lfs = std::numeric_limits<size_t>::max();
  lfsbasepath = NULL;
  for(size_t i = 0, ei = basepaths.size(); i != ei; i++)
    {
      size_t spaceavail;
      const string *basepath = &basepaths[i];

      fs::path::make(basepath,fusepath,fullpath);

      if(!fs::exists(fullpath))
        continue;
      if(!fs::spaceavail(*basepath,spaceavail))
        continue;
      if(spaceavail > lfs)
        continue;

      lfs = spaceavail;
      lfsbasepath = basepath;
    }

  if(lfsbasepath == NULL)
    return POLICY_FAIL_ENOENT;

  paths.push_back(lfsbasepath);

  return POLICY_SUCCESS;
}

static
int
_lfs(const Category::Enum::Type  type,
     const vector<string>       &basepaths,
     const char                 *fusepath,
     const size_t                minfreespace,
     vector<const string*>      &paths)
{
  if(type == Category::Enum::create)
    return _lfs_create(basepaths,minfreespace,paths);

  return _lfs_other(basepaths,fusepath,paths);
}


namespace mergerfs
{
  int
  Policy::Func::lfs(const Category::Enum::Type  type,
                    const vector<string>       &basepaths,
                    const char                 *fusepath,
                    const size_t                minfreespace,
                    vector<const string*>      &paths)
  {
    int rv;

    rv = _lfs(type,basepaths,fusepath,minfreespace,paths);
    if(POLICY_FAILED(rv))
      rv = Policy::Func::mfs(type,basepaths,fusepath,minfreespace,paths);

    return rv;
  }
}
