#ifndef _PHENOTYPE_H
#define _PHENOTYPE_H

///////////////////////////////////////////////////////////////////////////////////////////
//    MultiNEAT - Python/C++ NeuroEvolution of Augmenting Topologies Library
//
//    Copyright (C) 2012 Peter Chervenski
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with this program.  If not, see < http://www.gnu.org/licenses/ >.
//
//    Contact info:
//
//    Peter Chervenski < spookey@abv.bg >
//    Shane Ryan < shane.mcdonald.ryan@gmail.com >
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// File:        Phenotype.h
// Description: Definition for the phenotype data structures.
///////////////////////////////////////////////////////////////////////////////

#include <vector>
#include "Genes.h"
#include "Random.h"

#ifdef PYTHON_BINDINGS
    #include <pybind11/pybind11.h>
    #include <pybind11/numpy.h>
#endif

namespace NEAT
{

class Connection
{
public:
    int m_source_neuron_idx;       // index of source neuron
    int m_target_neuron_idx;       // index of target neuron
    double m_weight;                               // weight of the connection
    double m_signal;                               // weight * input signal

    bool m_recur_flag; // recurrence flag for displaying purposes
    // can be ignored

    // Hebbian learning parameters
    // Ignored in case there is no lifetime learning
    double m_hebb_rate;
    double m_hebb_pre_rate;

    // comparison operator (nessesary for python bindings)
    bool operator==(Connection const& other) const
    {
        if ((m_source_neuron_idx == other.m_source_neuron_idx) &&
            (m_target_neuron_idx == other.m_target_neuron_idx)) /*&&
            (m_weight == other.m_weight) &&
            (m_recur_flag == other.m_recur_flag))*/
            return true;
        else
            return false;
    }

};

class Neuron
{
public:
    double m_activesum;  // the synaptic input
    double m_activation; // the synaptic input passed through the activation function

    double m_a, m_b, m_timeconst, m_bias; // misc parameters
    double m_membrane_potential; // used in leaky integrator mode
    ActivationFunction m_activation_function_type;

    // displaying and stuff
    double m_x, m_y, m_z;
    double m_sx, m_sy, m_sz;
    std::vector<double> m_substrate_coords;
    double m_split_y;
    NeuronType m_type;

    // the sensitivity matrix of this neuron (for RTRL learning)
    std::vector< std::vector< double > > m_sensitivity_matrix;

    // comparison operator (nessesary for python bindings)
    bool operator==(Neuron const& other) const
    {
        if ((m_type == other.m_type) &&
            (m_split_y == other.m_split_y) &&
            (m_activation_function_type == other.m_activation_function_type)// &&
            //(this == other.this))
            )
            return true;
        else
            return false;
    }
};

class NeuralNetwork
{
    /////////////////////
    // RTRL variables
    double m_total_error;

    // Always the size of m_connections
    std::vector<double> m_total_weight_change;
    /////////////////////

    // returns the index if that connection exists or -1 otherwise
    int ConnectionExists(int a_to, int a_from);

public:

    unsigned int m_num_inputs, m_num_outputs;
    std::vector<Connection> m_connections; // array size - number of connections
    std::vector<Neuron>     m_neurons;

    NeuralNetwork(bool a_Minimal, RNG& random); // if given false, the constructor will create a standard XOR network topology.
    NeuralNetwork();

    void InitRTRLMatrix(); // initializes the sensitivity cube for RTRL learning.
    // assumes that neuron and connection data are already initialized

    /** Assumes unsigned sigmoids everywhere */
    void ActivateFast();

    /** Any activation function is supported */
    void Activate();

    /** Fully activate the NN. Infinite loop if the network has loops. Any activation function is supported. */
    void ActivateAllLayers();

    /** Like Activate() but uses m_bias as well */
    void ActivateUseInternalBias();

    /** Activates in leaky integrator mode */
    void ActivateLeaky(double step);

    void RTRL_update_gradients();
    void RTRL_update_error(double a_target);
    void RTRL_update_weights();   // performs the backprop step

    /** Hebbian learning */
    void Adapt(Parameters& a_Parameters);

    /** Clears all activations */
    void Flush();

    /** Clears the sensitivity cube */
    void FlushCube();

    void Input(const std::vector<double>& a_Inputs);

#ifdef PYTHON_BINDINGS
    void Input_python_list(const pybind11::list& a_Inputs);
    void Input_numpy(const pybind11::array_t<double, pybind11::array::c_style | pybind11::array::forcecast>& a_Inputs);
#endif

    std::vector<double> Output();

    // accessor methods
    void AddNeuron(const Neuron& a_n) { m_neurons.push_back( a_n ); }
    void AddConnection(const Connection& a_c) { m_connections.push_back( a_c ); }
    Connection GetConnectionByIndex(unsigned int a_idx) const
    {
        return m_connections[a_idx];
    }
    Neuron GetNeuronByIndex(unsigned int a_idx) const
    {
        return m_neurons[a_idx];
    }
    void SetInputOutputDimentions(const unsigned int a_i, const unsigned int a_o)
    {
        m_num_inputs = a_i;
        m_num_outputs = a_o;
    }
    unsigned int NumInputs() const
    {
        return m_num_inputs;
    }
    unsigned int NumOutputs() const
    {
        return m_num_outputs;
    }

    // clears the network and makes it a minimal one
    void Clear()
    {
        m_neurons.clear();
        m_connections.clear();
        m_total_weight_change.clear();
        SetInputOutputDimentions(0, 0);
    }

    double GetConnectionLenght(Neuron source, Neuron target)
    {   double dist = 0.0;
        for (unsigned int i = 0; i < source.m_substrate_coords.size(); i++)
        {
            dist += (target.m_substrate_coords[i] - source.m_substrate_coords[i]) *
            		(target.m_substrate_coords[i] - source.m_substrate_coords[i] );
        }
        return dist;
    }

    double GetTotalConnectionLength()
    {
        return m_connections.size();
    }

    unsigned int CalculateNetworkDepth() const;

    // one-shot save/load
    void Save(const char* a_filename);
    bool Load(const char* a_filename);

    // save/load from already opened files for reading/writing
    void Save(FILE* a_file);
    bool Load(std::ifstream& a_DataFile);
};

} // namespace NEAT




#endif
