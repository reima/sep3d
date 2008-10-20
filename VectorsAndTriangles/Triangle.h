#pragma once
#include "Vec3.h"

/**
 * Dreieck im 3-dimensionalen Raum.
 */
class Triangle
{
 public:
  /**
   * Konstruktor.
   * Erstellt ein Dreieck aus den gegebenen Eckpunkten.
   */
  Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3);
  
  /**
   * Gibt den Mittelpunkt des Dreiecks zurück.
   */
  Vec3 getCenter() const;
  
  /**
   * Gibt die Normale des Dreiecks zurück.
   * Der zurückgegebene Vektor ist bereits normalisiert.
   */   
  Vec3 getNormal() const;
  
  /**
   * Gibt eine Textrepräsentation des Dreiecks auf der Standardausgabe aus.
   */
  void print() const;

 private:
  /**
   * Die 3 Eckpunkte des Dreiecks.
   */
  Vec3 p1_, p2_, p3_;
};
