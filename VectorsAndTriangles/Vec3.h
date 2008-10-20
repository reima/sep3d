#pragma once

/**
 * 3-dimensionaler Vektor.
 */
class Vec3 {
 public:
  /**
   * Konstruktor.
   * Initialisiert den Vektor auf die angegebenen Koordinaten.
   */
  Vec3(double x, double y, double z);

  /**
   * Addiert einen Vektor.
   */
  void add(const Vec3 &v);

  /**
   * Subtrahiert einen Vektor.
   */
  void subtract(const Vec3 &v);

  /**
   * Skaliert den Vektor.
   */
  void scale(double f);

  /**
   * Gibt die Norm (Länge) des Vektors zurück.
   */
  double getNorm() const;

  /**
   * Normalisiert den Vektor.
   * @warning Wird versucht, den Nullvektor zu normalisieren, kommt es zu einer
   *          Division durch 0.
   */
  void normalize();

  /**
   * Testet den Vektor auf Gleichheit mit einem anderen Vektor.
   * @warning Der Gleichheitstest erfolgt exakt (Ungenauigkeit der
   *          Fließkommadarstellung wird nicht berücksichtigt).
   */
  bool isEqual(const Vec3 &v) const;

  /**
   * Berechnet das Kreuzprodukt mit einem anderen Vektor.
   */
  Vec3 crossProd(const Vec3 &v) const;

  /**
   * Gibt eine Textrepräsentation des Vektors auf der Standardausgabe aus.
   */
  void print() const;

 private:
  /**
   * Die 3 Komponenten des Vektors.
   */
  double x_, y_, z_;
};
