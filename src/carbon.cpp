#include <Rcpp.h>
#include "hydraulics.h"
using namespace Rcpp;


const double glucoseMolarWeight = 180.156; //g*mol-1
const double starchMolarWeight = 162.1406; //g*mol-1
const double starchDensity = 1.5; //g·cm-3

const double leafCperDry = 0.3; //g C · g dry-1
const double rootCperDry = 0.4959; //g C · g dry-1

const double Rn = 0.008314472; // The perfect gas constant MPa·l/K·mol = kJ/K·mol

/**
 * Van 't Hoff equation
 *  conc - mol/l 
 *  temp - deg C
 *  wp - MPa
 */
// [[Rcpp::export("moisture_osmoticWaterPotential")]]
double osmoticWaterPotential(double conc, double temp) {
  return(- conc*Rn*(temp + 273.15));
}
// [[Rcpp::export("moisture_sugarConcentration")]]
double sugarConcentration(double osmoticWP, double temp) {
  return(- osmoticWP/(Rn*(temp + 273.15)));
}


/**
* On the pressure dependence of the viscosity of aqueous sugar solutions
* Rheol Acta (2002) 41: 369–374 DOI 10.1007/s00397-002-0238-y
* 
*  x - sugar concentration (mol/l)
*  temp - temperature (degrees C)
*/
// [[Rcpp::export("carbon_relativeSapViscosity")]]
double relativeSapViscosity(double conc, double temp) {
  double x = conc*glucoseMolarWeight/1e3; //from mol/l to g*cm-3
  double Tkelvin = temp + 273.15;
  double q0a = 1.12; //g*cm-3
  double q1 = -0.248;
  double Ea = 2.61; //kJ*mol-1 energy of activation
  double va = x/(q0a*exp(-1.0*Ea/(Rn*Tkelvin)));
  double relVisc = exp(va/(1.0 + q1*va)); // relative viscosity
  double relWat = exp(-3.7188+(578.919/(-137.546+ Tkelvin))); // Vogel equation for liquid dynamic viscosity (= 1 for 25ºC)
  return(relWat*relVisc);
}

/**
 *  Turgor (MPa)
 *  conc - mol/l 
 *  temp - deg C
 *  psi - water potential (MPa)
 */
// [[Rcpp::export("carbon_turgor")]]
double turgor(double psi, double conc, double temp) {
  return(std::max(0.0, psi-osmoticWaterPotential(conc,temp)));
}


double leafArea(double LAI, double N) {
  return(10000.0*LAI/N); //Leaf area in m2 · ind-1
}
/**
 * leaf volume in l
 */
// [[Rcpp::export("carbon_leafStorageVolume")]]
double leafStorageVolume(double LAI, double N, double SLA, double leafDensity) {
  return(leafArea(LAI,N)*leafWaterCapacity(SLA, leafDensity)); 
}

// [[Rcpp::export("carbon_leafCstructural")]]
double leafCstructural(double LAI, double N, double SLA) {
  return(leafCperDry*1000.0*leafArea(LAI,N)/SLA);  //Leaf structural biomass in g C · ind-1
}
/**
 * sapwood volume in l
 */
double sapwoodVolume(double SA, double H, double Z) { //SA in cm2, H and Z in cm
  return(0.001*SA*(H+Z));
}
/**
 * sapwood storage volume in l
 */
// [[Rcpp::export("carbon_sapwoodStorageVolume")]]
double sapwoodStorageVolume(double SA, double H, double Z, double woodDensity, double vessel2sapwood) { //SA in cm2, H and Z in cm
  double woodPorosity = (1.0- (woodDensity/1.54));
  return((1.0 - vessel2sapwood)*sapwoodVolume(SA,H,Z)*woodPorosity);
}
// [[Rcpp::export("carbon_sapwoodCstructural")]]
double sapwoodCstructural(double SA, double H, double Z, double woodDensity, double woodCperDry) {//SA in cm2, H and Z in cm
  return(woodCperDry*sapwoodVolume(SA,H,Z)*woodDensity);
}
/*
 * Leaf starch storage capacity in g C · ind-1
 * Up to 10% of leaf cell volume
 */
// [[Rcpp::export("carbon_leafStarchCapacity")]]
double leafStarchCapacity(double LAI, double N, double SLA, double leafDensity) {
  return(0.1*1000.0*leafStorageVolume(LAI,N,SLA,leafDensity)*starchDensity);
}

/*
 *  Sapwood starch storage capacity in g C · ind-1
 *  Up to 50% of volume of non-conductive cells
 */
// [[Rcpp::export("carbon_sapwoodStarchCapacity")]]
double sapwoodStarchCapacity(double SA, double H, double Z, double woodDensity, double vessel2sapwood) {
  return(0.5*1000.0*sapwoodStorageVolume(SA,H,Z,woodDensity,vessel2sapwood)*starchDensity);
}

NumericVector carbonCompartments(double SA, double LAI, double H, double Z, double N, double SLA, double WoodDensity, double WoodC) {
  double B_leaf = leafCstructural(LAI,N,SLA);
  double B_stem = sapwoodCstructural(SA,H,Z,WoodDensity, WoodC);
  double B_fineroot = B_leaf/2.5;
  return(NumericVector::create(B_leaf, B_stem, B_fineroot)); 
}