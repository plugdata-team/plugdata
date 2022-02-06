/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace pd
{
class Instance;
class Gui;
// ==================================================================================== //
//                                          GRAPH                                       //
// ==================================================================================== //

//! @brief The Pd array.
//! @details The class is a wrapper around a Pd array. The lifetime of the internal array\n
//! is not guaranteed by the class.
//! @see Instance, Gui
class Array
{
 public:
  Array(std::string name, void* instance);

  //! @brief The default constructor.
  Array() = default;

  //! @brief Gets the name of the array.
  std::string getName() const noexcept;

  //! @brief Gets id it should be drawn as points.
  bool isDrawingPoints() const noexcept;

  //! @brief Gets id it should be drawn as lines.
  bool isDrawingLine() const noexcept;

  //! @brief Gets id it should be drawn as curves.
  bool isDrawingCurve() const noexcept;

  //! @brief Gets the scale of the array.
  std::array<float, 2> getScale() const noexcept;

  //! @brief Gets the values of the array.
  void read(std::vector<float>& output) const;

  //! @brief Writes the values of the array.
  void write(std::vector<float> const& input);

  //! @brief Writes a value of the array.
  void write(const size_t pos, float const input);

 private:
  std::string name = std::string("");
  void* instance = nullptr;

  friend class Instance;
  friend class Gui;
};
}  // namespace pd
