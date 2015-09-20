/*
 * This file is part of the SPLINTER library.
 * Copyright (C) 2012 Bjarne Grimstad (bjarne.grimstad@gmail.com).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "polynomial.h"
#include <serializer.h>
#include "mykroneckerproduct.h"

namespace SPLINTER
{

Polynomial::Polynomial(const char *fileName)
    : Polynomial(std::string(fileName))
{
}

Polynomial::Polynomial(const std::string fileName)
    : LinearFunction(1, DenseVector::Zero(1))
{
    load(fileName);
}

Polynomial::Polynomial(unsigned int numVariables, unsigned int degree)
    : Polynomial(std::vector<unsigned int>(numVariables, degree))
{
}

// Initialize coefficients to zero
Polynomial::Polynomial(std::vector<unsigned int> degrees)
    : LinearFunction(degrees.size(), DenseVector::Zero(computeNumBasisFunctions(degrees))),
      degrees(degrees)
{
}

Polynomial::Polynomial(std::vector<unsigned int> degrees, DenseVector coefficients)
    : LinearFunction(degrees.size(), coefficients),
      degrees(degrees)
{
}

unsigned int Polynomial::computeNumBasisFunctions(std::vector<unsigned int> degrees) const
{
    unsigned int numMonomials = 1;
    for (auto deg : degrees)
        numMonomials *= (deg+1);

    return numMonomials;
}

/**
 * Evaluate monomials
 */
SparseVector Polynomial::evalBasisFunctions(DenseVector x) const
{
    std::vector<DenseVector> powers;

    for (unsigned int i = 0; i < numVariables; ++i)
    {
        unsigned int deg = degrees.at(i);
        DenseVector powi = DenseVector::Zero(deg+1);
        for (unsigned int j = 0; j <= deg; ++j)
            powi(j) = std::pow(x(i), j);
        powers.push_back(powi);
    }

    // Kronecker product of monovariable power basis
    DenseVector monomials = kroneckerProductVectors(powers);

    if (monomials.rows() != getNumCoefficients())
    {
        throw Exception("PolynomialApproximant::evalMonomials: monomials.rows() != numCoefficients.");
    }

    return monomials.sparseView();
}

SparseMatrix Polynomial::evalBasisFunctionsJacobian(DenseVector x) const
{
    DenseMatrix jac = DenseMatrix::Zero(getNumCoefficients(), numVariables);

    for (unsigned int var = 0; var < numVariables; ++var)
        jac.block(0,var,getNumCoefficients(),1) = evalDifferentiatedMonomials(x, var);

    return jac.sparseView();
}

DenseVector Polynomial::evalDifferentiatedMonomials(DenseVector x, unsigned int var) const
{
    if (var < 0 || var >= numVariables)
        throw Exception("PolynomialApproximant::evalDifferentiatedMonomials: invalid variable.");

    std::vector<DenseVector> powers;

    for (unsigned int i = 0; i < numVariables; ++i)
    {
        unsigned int deg = degrees.at(i);
        DenseVector powi = DenseVector::Zero(deg+1);

        if (var == i)
        {
            // Differentiate wrt. x(i)
            for (unsigned int j = 1; j <= deg; ++j)
                powi(j) = j*std::pow(x(i), j-1);
        }
        else
        {
            for (unsigned int j = 0; j <= deg; ++j)
                powi(j) = std::pow(x(i), j);
        }

        powers.push_back(powi);
    }

    // Kronecker product of monovariable power basis
    DenseVector monomials = kroneckerProductVectors(powers);

    if (monomials.rows() != getNumCoefficients())
        throw Exception("PolynomialApproximant::evalMonomials: monomials.rows() != numCoefficients.");

    return monomials;
}

void Polynomial::save(const std::string fileName) const
{
    Serializer s;
    s.serialize(*this);
    s.saveToFile(fileName);
}

void Polynomial::load(const std::string fileName)
{
    Serializer s(fileName);
    s.deserialize(*this);
}

const std::string Polynomial::getDescription() const
{
    std::string description("PolynomialApproximant of degree");

    // See if all degrees are the same.
    bool equal = true;
    for (size_t i = 1; i < degrees.size(); ++i)
    {
        equal = equal && (degrees.at(i) == degrees.at(i-1));
    }

    if (equal)
    {
        description.append(" ");
        description.append(std::to_string(degrees.at(0)));
    }
    else
    {
        description.append("s (");
        for (size_t i = 0; i < degrees.size(); ++i) {
            description.append(std::to_string(degrees.at(i)));
            if (i + 1 < degrees.size())
            {
                description.append(", ");
            }
        }
        description.append(")");
    }

    return description;
}

} // namespace SPLINTER