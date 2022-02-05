/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdArray.h"

extern "C"
{
#include "x_libpd_extra_utils.h"
}

namespace pd
{
// ==================================================================================== //
//                                      GRAPH                                           //
// ==================================================================================== //

Array::Array(std::string arrayName, void *arrayInstance) : name(std::move(arrayName)), instance(arrayInstance) {}

std::string Array::getName() const noexcept { return name; }

bool Array::isDrawingPoints() const noexcept
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  return libpd_array_get_style(name.c_str()) == 0;
}

bool Array::isDrawingLine() const noexcept
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  return libpd_array_get_style(name.c_str()) == 1;
}

bool Array::isDrawingCurve() const noexcept
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  return libpd_array_get_style(name.c_str()) == 2;
}

std::array<float, 2> Array::getScale() const noexcept
{
  float min = -1, max = 1;
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  libpd_array_get_scale(name.c_str(), &min, &max);
  return {min, max};
}

void Array::read(std::vector<float> &output) const
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  int const size = libpd_arraysize(name.c_str());
  output.resize(static_cast<size_t>(size));
  libpd_read_array(output.data(), name.c_str(), 0, size);
}

void Array::write(std::vector<float> const &input)
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  libpd_write_array(name.c_str(), 0, input.data(), static_cast<int>(input.size()));
}

void Array::write(const size_t pos, float const input)
{
  libpd_set_instance(static_cast<t_pdinstance *>(instance));
  libpd_write_array(name.c_str(), static_cast<int>(pos), &input, 1);
}
}  // namespace pd
