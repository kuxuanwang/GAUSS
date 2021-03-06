#ifndef ENERGY_MUSCLE
#define ENERGY_MUSCLE

#include<cmath>

template<typename DataType>
inline DataType musclePow(DataType a, DataType b) {
        
    return static_cast<DataType> (std::pow(std::cbrt(static_cast<double>(a)),static_cast<double>(b)));
            
}

template<typename DataType, typename ShapeFunction>
class EnergyMuscle : public virtual ShapeFunction {
public:
    template<typename QDOFList, typename QDotDOFList>
    EnergyMuscle(Eigen::MatrixXd &V, Eigen::MatrixXi &F, QDOFList &qDOFList, QDotDOFList &qDotDOFList) : ShapeFunction(V, F, qDOFList, qDotDOFList) {
        setParameters(2e5, 0.45);
        muscle_magnitude = 0;
        fibre_direction(0) = 0;
        fibre_direction(1) = 0;
        fibre_direction(2) = 0;
        
    }
    
    inline void setParameters(double youngsModulus, double poissonsRatio) {
        m_C = youngsModulus/(2.0*(1.0+poissonsRatio));
        m_D = (youngsModulus*poissonsRatio)/((1.0+poissonsRatio)*(1.0-2.0*poissonsRatio));
    
    }

    inline void setMuscleParameters(double muslc_mag, Eigen::Vector3d &fibre_dir){
        muscle_magnitude = muslc_mag;
        fibre_direction = fibre_dir;
    }
    
    DataType getValue(double *x, const State<DataType> &state) {
    
        Eigen::Matrix<DataType, 3,3> F = ShapeFunction::F(x,state) + Eigen::Matrix<DataType,3,3>::Identity();
        double detF = F.determinant();
        double J23 = musclePow(detF,2.0);
        J23=1.0/J23;
        //double J23 = 1.0/(std::pow(detF*detF, 1.0/3.0));
        Eigen::Matrix<DataType, 3,3> Cbar = J23*F.transpose()*F;
        double neo_e =  m_C*(Cbar.trace() - 3.0) + m_D*(detF - 1)*(detF - 1);
        double musc_e =  (fibre_direction.transpose()*F.transpose()*F*fibre_direction);
        return 0.5*muscle_magnitude*musc_e + neo_e;

        //stable neohookean flesh
        // Eigen::Matrix<DataType, 3,3> F = ShapeFunction::F(x,state) + Eigen::Matrix<DataType,3,3>::Identity();
        // double J = F.determinant();
        // double Ic = (F.transpose()*F).trace();
        // double alpha = (1+(m_C/m_D) - m_D*(m_C/4.0));
        // double stable_neo_energy = 0.5*m_C*(Ic - 3) + 0.5*m_D*(J - alpha)*(J-alpha) - 0.5*m_C*log(Ic + 1);
        // double muscle_energy = 0.5*muscle_magnitude*(fibre_direction.transpose()*F.transpose()*F*fibre_direction - 1);
        // return muscle_energy + stable_neo_energy;
    }
    
    template<typename Vector>
    inline void getGradient(Vector &f, double *x, const State<DataType> &state) {
        //Deformation Gradient
        double f11, f12, f13, f21, f22, f23, f31, f32, f33;
        
        Eigen::Matrix<DataType, 3,3> F = ShapeFunction::F(x,state);
        
        f11 = F(0,0)+1.0;
        f12 = F(0,1);
        f13 = F(0,2);
        f21 = F(1,0);
        f22 = F(1,1)+1.0;
        f23 = F(1,2);
        f31 = F(2,0);
        f32 = F(2,1);
        f33 = F(2,2)+1.0;
        
        //Force Vector computation from Mathematica notebook
        Eigen::Matrix<DataType, 9,1> dw;
        double C,D;
        C = m_C;
        D = m_D +C;
        
        double v1, v2, v3;
        v1 = fibre_direction(0);
        v2 = fibre_direction(1);
        v3 = fibre_direction(2);

        dw[0] = v1* (f11 * v1 + f12 * v2 + f13 * v3)* muscle_magnitude;
        dw[1] = v2* (f11 * v1 + f12 * v2 + f13 * v3)* muscle_magnitude;
        dw[2] = v3* (f11 * v1 + f12 * v2 + f13 * v3)* muscle_magnitude;
        dw[3] = v1* (f21 * v1 + f22 * v2 + f23 * v3)* muscle_magnitude;
        dw[4] = v2* (f21 * v1 + f22 * v2 + f23 * v3)* muscle_magnitude;
        dw[5] = v3* (f21 * v1 + f22 * v2 + f23 * v3)* muscle_magnitude;
        dw[6] = v1* (f31 * v1 + f32 * v2 + f33 * v3)* muscle_magnitude;
        dw[7] = v2* (f31 * v1 + f32 * v2 + f33 * v3)* muscle_magnitude;
        dw[8] = v3* (f31 * v1 + f32 * v2 + f33 * v3)* muscle_magnitude;

        //STABLE NEOHOOKEAN
            dw[0] += 1.*C*f11 - (1.*C*f11)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 1.*(f23*f32 - 1.*f22*f33)*(0.75*C + D*(1 + f13*f22*f31 - 1.*f12*f23*f31 - 1.*f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - 1.*f11*f22*f33));
            dw[1] += 1.*C*f12 + 1.*D*(f23*f31 - f21*f33)*(-1 - (0.75*C)/D - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - (1.*C*f12)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));
            dw[2] += 1.*C*f13 - (1.*C*f13)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 1.*(f22*f31 - 1.*f21*f32)*(0.75*C + D*(1 + f13*f22*f31 - 1.*f12*f23*f31 - 1.*f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - 1.*f11*f22*f33));
            dw[3] += 1.*C*f21 + 1.*D*(f13*f32 - f12*f33)*(-1 - (0.75*C)/D - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - (1.*C*f21)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));
            dw[4] += 1.*C*f22 - (1.*C*f22)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 1.*(f13*f31 - 1.*f11*f33)*(0.75*C + D*(1 + f13*f22*f31 - 1.*f12*f23*f31 - 1.*f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - 1.*f11*f22*f33));
            dw[5] += 1.*C*f23 + 1.*D*(f12*f31 - f11*f32)*(-1 - (0.75*C)/D - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - (1.*C*f23)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));
            dw[6] += 1.*C*f31 - (1.*C*f31)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 1.*(f13*f22 - 1.*f12*f23)*(0.75*C + D*(1 + f13*f22*f31 - 1.*f12*f23*f31 - 1.*f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - 1.*f11*f22*f33));
            dw[7] += 1.*C*f32 + 1.*D*(f13*f21 - f11*f23)*(-1 - (0.75*C)/D - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - (1.*C*f32)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));
            dw[8] += 1.*C*f33 - (1.*C*f33)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 1.*(f12*f21 - 1.*f11*f22)*(0.75*C + D*(1 + f13*f22*f31 - 1.*f12*f23*f31 - 1.*f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - 1.*f11*f22*f33));
        
        // //REGULAR NH
        //     C = 0.5*C;
        //     D = 0.5*D;
        //     dw[0] += 2*D*(f23*f32 - f22*f33)*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(3*f11*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f12*f21*f33) + std::pow(f11,2)*(-2*f23*f32 + 2*f22*f33) + (f23*f32 - f22*f33)*(std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[1] += 2*D*(f23*f31 - f21*f33)*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - (2*C*(std::pow(f12,2)*(-2*f23*f31 + 2*f21*f33) + 3*f12*(f13*f22*f31 - f13*f21*f32 + f11*f23*f32 - f11*f22*f33) + (f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[2] += 2*D*(f22*f31 - f21*f32)*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*f13)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) + (2*C*(f22*f31 - f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[3] += 2*D*(f13*f32 - f12*f33)*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*f21)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) - (2*C*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[4] += 2*D*(f13*f31 - f11*f33)*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*f22)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) + (2*C*(f13*f31 - f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[5] += 2*D*(f12*f31 - f11*f32)*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*f23)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) - (2*C*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[6] += 2*D*(f13*f22 - f12*f23)*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*f31)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) + (2*C*(f13*f22 - f12*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2.0) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[7] += 2*D*(f13*f21 - f11*f23)*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*f32)/musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,2.0) - (2*C*(f13*f21 - f11*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));
        //     dw[8] += 2*D*(f12*f21 - f11*f22)*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - (2*C*((-(f12*f21) + f11*f22)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2)) + 3*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32)*f33 + 2*(f12*f21 - f11*f22)*std::pow(f33,2)))/(3.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,5.0));


        f = -ShapeFunction::GradJ(0,x,state).transpose()*dw.segment(0,3) +
            -ShapeFunction::GradJ(1,x,state).transpose()*dw.segment(3,3) +
            -ShapeFunction::GradJ(2,x,state).transpose()*dw.segment(6,3);        
        
    }
    
    template<typename Matrix>
    inline void getHessian(Matrix &H, double *x, const State<DataType> &state) {
        //H = -ShapeFunction::B(x,state).transpose()*m_C*ShapeFunction::B(x,state);
        Eigen::Matrix<DataType, 3,3> F = ShapeFunction::F(x,state);
        double f11, f12, f13, f21, f22, f23, f31, f32, f33;
        
        f11 = F(0,0)+1.0;
        f12 = F(0,1);
        f13 = F(0,2);
        f21 = F(1,0);
        f22 = F(1,1)+1.0;
        f23 = F(1,2);
        f31 = F(2,0);
        f32 = F(2,1);
        f33 = F(2,2)+1.0;
        
        //H = -B(this, x, state).transpose()*m_C*B(this, x, state);
        Eigen::Matrix<DataType,9,9> ddw;
        typename ShapeFunction::MatrixJ gradX, gradY, gradZ;
        gradX = ShapeFunction::GradJ(0,x,state);
        gradY = ShapeFunction::GradJ(1,x,state);
        gradZ = ShapeFunction::GradJ(2,x,state);
        
        double C,D;
        C = m_C;
        D = m_D;
        
        double v1, v2, v3;
        v1 = fibre_direction(0);
        v2 = fibre_direction(1);
        v3 = fibre_direction(2);
        
        // //Stable Neohookean
            ddw(0,0) = 1.*C + 1.*D*std::pow(f23*f32 - 1.*f22*f33,2) + (2.*C*std::pow(f11,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(1,0) = -1.*D*(f23*f31 - 1.*f21*f33)*(f23*f32 - 1.*f22*f33) + (2.*C*f11*f12)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(2,0) = -1.*D*(f22*f31 - 1.*f21*f32)*(-1.*f23*f32 + f22*f33) + (2.*C*f11*f13)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(3,0) = -1.*D*(f13*f32 - 1.*f12*f33)*(f23*f32 - 1.*f22*f33) + (2.*C*f11*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(4,0) = -0.75*C*f33 + D*f33*(-1. + 1.*f12*f23*f31 - 2.*f11*f23*f32 - 1.*f12*f21*f33 + 2.*f11*f22*f33) + D*f13*(1.*f23*f31*f32 - 2.*f22*f31*f33 + 1.*f21*f32*f33) + (2.*C*f11*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(5,0) = D*f32*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 - 1.*f13*f21*f32 + 2.*f11*f23*f32) + D*(1.*f12*f22*f31 + 1.*f12*f21*f32 - 2.*f11*f22*f32)*f33 + C*(0.75*f32 + (2.*f11*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(6,0) = -1.*D*(f13*f22 - 1.*f12*f23)*(-1.*f23*f32 + f22*f33) + (2.*C*f11*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(7,0) = D*f23*(1. + 1.*f13*f22*f31 - 1.*f12*f23*f31 - 2.*f13*f21*f32 + 2.*f11*f23*f32) + D*(1.*f13*f21*f22 + 1.*f12*f21*f23 - 2.*f11*f22*f23)*f33 + C*(0.75*f23 + (2.*f11*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(8,0) = D*(1.*f12*f21*f23*f32 + std::pow(f22,2)*(-1.*f13*f31 + 2.*f11*f33) + f22*(-1. + 1.*f12*f23*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32 - 2.*f12*f21*f33)) + C*(-0.75*f22 + (2.*f11*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(0,1) = -1.*D*(f23*f31 - 1.*f21*f33)*(f23*f32 - 1.*f22*f33) + (2.*C*f11*f12)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(1,1) = 1.*C + 1.*D*std::pow(f23*f31 - 1.*f21*f33,2) + (2.*C*std::pow(f12,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(2,1) = -1.*D*(f22*f31 - 1.*f21*f32)*(f23*f31 - 1.*f21*f33) + (2.*C*f12*f13)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(3,1) = 0.75*C*f33 + D*f33*(1. - 2.*f12*f23*f31 + 1.*f11*f23*f32 + 2.*f12*f21*f33 - 1.*f11*f22*f33) + D*f13*(1.*f23*f31*f32 + 1.*f22*f31*f33 - 2.*f21*f32*f33) + (2.*C*f12*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(4,1) = -1.*D*(f13*f31 - 1.*f11*f33)*(f23*f31 - 1.*f21*f33) + (2.*C*f12*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(5,1) = D*f31*(-1. - 1.*f13*f22*f31 + 2.*f12*f23*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32) + D*(-2.*f12*f21*f31 + 1.*f11*f22*f31 + 1.*f11*f21*f32)*f33 + C*(-0.75*f31 + (2.*f12*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(6,1) = D*f23*(-1. - 2.*f13*f22*f31 + 2.*f12*f23*f31 + 1.*f13*f21*f32 - 1.*f11*f23*f32) + D*(1.*f13*f21*f22 - 2.*f12*f21*f23 + 1.*f11*f22*f23)*f33 + C*(-0.75*f23 + (2.*f12*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(7,1) = -1.*D*(f13*f21 - 1.*f11*f23)*(-1.*f23*f31 + f21*f33) + (2.*C*f12*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(8,1) = D*(1.*f11*f22*f23*f31 + std::pow(f21,2)*(-1.*f13*f32 + 2.*f12*f33) + f21*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 + 1.*f11*f23*f32 - 2.*f11*f22*f33)) + C*(0.75*f21 + (2.*f12*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(0,2) = -1.*D*(f22*f31 - 1.*f21*f32)*(-1.*f23*f32 + f22*f33) + (2.*C*f11*f13)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(1,2) = -1.*D*(f22*f31 - 1.*f21*f32)*(f23*f31 - 1.*f21*f33) + (2.*C*f12*f13)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(2,2) = 1.*C + 1.*D*std::pow(f22*f31 - 1.*f21*f32,2) + (2.*C*std::pow(f13,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(3,2) = D*f32*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 + 2.*f13*f21*f32 - 1.*f11*f23*f32) + D*(1.*f12*f22*f31 - 2.*f12*f21*f32 + 1.*f11*f22*f32)*f33 + C*(-0.75*f32 + (2.*f13*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(4,2) = D*f31*(1. + 2.*f13*f22*f31 - 1.*f12*f23*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32) + D*(1.*f12*f21*f31 - 2.*f11*f22*f31 + 1.*f11*f21*f32)*f33 + C*(0.75*f31 + (2.*f13*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(5,2) = -1.*D*(f12*f31 - 1.*f11*f32)*(f22*f31 - 1.*f21*f32) + (2.*C*f13*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(6,2) = D*(1.*f12*f21*f23*f32 + std::pow(f22,2)*(2.*f13*f31 - 1.*f11*f33) + f22*(1. - 2.*f12*f23*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32 + 1.*f12*f21*f33)) + C*(0.75*f22 + (2.*f13*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(7,2) = D*(1.*f11*f22*f23*f31 + std::pow(f21,2)*(2.*f13*f32 - 1.*f12*f33) + f21*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 - 2.*f11*f23*f32 + 1.*f11*f22*f33)) + C*(-0.75*f21 + (2.*f13*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(8,2) = -1.*D*(f12*f21 - 1.*f11*f22)*(-1.*f22*f31 + f21*f32) + (2.*C*f13*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(0,3) = -1.*D*(f13*f32 - 1.*f12*f33)*(f23*f32 - 1.*f22*f33) + (2.*C*f11*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(1,3) = 0.75*C*f33 + D*f33*(1. - 2.*f12*f23*f31 + 1.*f11*f23*f32 + 2.*f12*f21*f33 - 1.*f11*f22*f33) + D*f13*(1.*f23*f31*f32 + 1.*f22*f31*f33 - 2.*f21*f32*f33) + (2.*C*f12*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(2,3) = D*f32*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 + 2.*f13*f21*f32 - 1.*f11*f23*f32) + D*(1.*f12*f22*f31 - 2.*f12*f21*f32 + 1.*f11*f22*f32)*f33 + C*(-0.75*f32 + (2.*f13*f21)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(3,3) = 1.*C + 1.*D*std::pow(f13*f32 - 1.*f12*f33,2) + (2.*C*std::pow(f21,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(4,3) = -1.*D*(f13*f31 - 1.*f11*f33)*(f13*f32 - 1.*f12*f33) + (2.*C*f21*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(5,3) = -1.*D*(f12*f31 - 1.*f11*f32)*(-1.*f13*f32 + f12*f33) + (2.*C*f21*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(6,3) = -1.*D*(f13*f22 - 1.*f12*f23)*(f13*f32 - 1.*f12*f33) + (2.*C*f21*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(7,3) = D*f13*(-1. - 1.*f13*f22*f31 + 1.*f12*f23*f31 + 2.*f13*f21*f32 - 2.*f11*f23*f32) + D*(-2.*f12*f13*f21 + 1.*f11*f13*f22 + 1.*f11*f12*f23)*f33 + C*(-0.75*f13 + (2.*f21*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(8,3) = D*(1.*f11*f13*f22*f32 + std::pow(f12,2)*(-1.*f23*f31 + 2.*f21*f33) + f12*(1. + 1.*f13*f22*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32 - 2.*f11*f22*f33)) + C*(0.75*f12 + (2.*f21*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(0,4) = -0.75*C*f33 + D*f33*(-1. + 1.*f12*f23*f31 - 2.*f11*f23*f32 - 1.*f12*f21*f33 + 2.*f11*f22*f33) + D*f13*(1.*f23*f31*f32 - 2.*f22*f31*f33 + 1.*f21*f32*f33) + (2.*C*f11*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(1,4) = -1.*D*(f13*f31 - 1.*f11*f33)*(f23*f31 - 1.*f21*f33) + (2.*C*f12*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(2,4) = D*f31*(1. + 2.*f13*f22*f31 - 1.*f12*f23*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32) + D*(1.*f12*f21*f31 - 2.*f11*f22*f31 + 1.*f11*f21*f32)*f33 + C*(0.75*f31 + (2.*f13*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(3,4) = -1.*D*(f13*f31 - 1.*f11*f33)*(f13*f32 - 1.*f12*f33) + (2.*C*f21*f22)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(4,4) = 1.*C + 1.*D*std::pow(f13*f31 - 1.*f11*f33,2) + (2.*C*std::pow(f22,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(5,4) = -1.*D*(f12*f31 - 1.*f11*f32)*(f13*f31 - 1.*f11*f33) + (2.*C*f22*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(6,4) = D*f13*(1. + 2.*f13*f22*f31 - 2.*f12*f23*f31 - 1.*f13*f21*f32 + 1.*f11*f23*f32) + D*(1.*f12*f13*f21 - 2.*f11*f13*f22 + 1.*f11*f12*f23)*f33 + C*(0.75*f13 + (2.*f22*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(7,4) = -1.*D*(f13*f21 - 1.*f11*f23)*(f13*f31 - 1.*f11*f33) + (2.*C*f22*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(8,4) = 1.*D*f12*f13*f21*f31 + D*f11*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 + 1.*f13*f21*f32 - 1.*f11*f23*f32 - 2.*f12*f21*f33 + 2.*f11*f22*f33) + C*(-0.75*f11 + (2.*f22*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(0,5) = D*f32*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 - 1.*f13*f21*f32 + 2.*f11*f23*f32) + D*(1.*f12*f22*f31 + 1.*f12*f21*f32 - 2.*f11*f22*f32)*f33 + C*(0.75*f32 + (2.*f11*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(1,5) = D*f31*(-1. - 1.*f13*f22*f31 + 2.*f12*f23*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32) + D*(-2.*f12*f21*f31 + 1.*f11*f22*f31 + 1.*f11*f21*f32)*f33 + C*(-0.75*f31 + (2.*f12*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(2,5) = -1.*D*(f12*f31 - 1.*f11*f32)*(f22*f31 - 1.*f21*f32) + (2.*C*f13*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(3,5) = -1.*D*(f12*f31 - 1.*f11*f32)*(-1.*f13*f32 + f12*f33) + (2.*C*f21*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(4,5) = -1.*D*(f12*f31 - 1.*f11*f32)*(f13*f31 - 1.*f11*f33) + (2.*C*f22*f23)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(5,5) = 1.*C + 1.*D*std::pow(f12*f31 - 1.*f11*f32,2) + (2.*C*std::pow(f23,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(6,5) = D*(1.*f11*f13*f22*f32 + std::pow(f12,2)*(2.*f23*f31 - 1.*f21*f33) + f12*(-1. - 2.*f13*f22*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32 + 1.*f11*f22*f33)) + C*(-0.75*f12 + (2.*f23*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(7,5) = 1.*D*f12*f13*f21*f31 + D*f11*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 - 2.*f13*f21*f32 + 2.*f11*f23*f32 + 1.*f12*f21*f33 - 1.*f11*f22*f33) + C*(0.75*f11 + (2.*f23*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(8,5) = -1.*D*(f12*f21 - 1.*f11*f22)*(f12*f31 - 1.*f11*f32) + (2.*C*f23*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(0,6) = -1.*D*(f13*f22 - 1.*f12*f23)*(-1.*f23*f32 + f22*f33) + (2.*C*f11*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(1,6) = D*f23*(-1. - 2.*f13*f22*f31 + 2.*f12*f23*f31 + 1.*f13*f21*f32 - 1.*f11*f23*f32) + D*(1.*f13*f21*f22 - 2.*f12*f21*f23 + 1.*f11*f22*f23)*f33 + C*(-0.75*f23 + (2.*f12*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(2,6) = D*(1.*f12*f21*f23*f32 + std::pow(f22,2)*(2.*f13*f31 - 1.*f11*f33) + f22*(1. - 2.*f12*f23*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32 + 1.*f12*f21*f33)) + C*(0.75*f22 + (2.*f13*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(3,6) = -1.*D*(f13*f22 - 1.*f12*f23)*(f13*f32 - 1.*f12*f33) + (2.*C*f21*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(4,6) = D*f13*(1. + 2.*f13*f22*f31 - 2.*f12*f23*f31 - 1.*f13*f21*f32 + 1.*f11*f23*f32) + D*(1.*f12*f13*f21 - 2.*f11*f13*f22 + 1.*f11*f12*f23)*f33 + C*(0.75*f13 + (2.*f22*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(5,6) = D*(1.*f11*f13*f22*f32 + std::pow(f12,2)*(2.*f23*f31 - 1.*f21*f33) + f12*(-1. - 2.*f13*f22*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32 + 1.*f11*f22*f33)) + C*(-0.75*f12 + (2.*f23*f31)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(6,6) = 1.*C + 1.*D*std::pow(f13*f22 - 1.*f12*f23,2) + (2.*C*std::pow(f31,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(7,6) = -1.*D*(f13*f21 - 1.*f11*f23)*(f13*f22 - 1.*f12*f23) + (2.*C*f31*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(8,6) = -1.*D*(f12*f21 - 1.*f11*f22)*(-1.*f13*f22 + f12*f23) + (2.*C*f31*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(0,7) = D*f23*(1. + 1.*f13*f22*f31 - 1.*f12*f23*f31 - 2.*f13*f21*f32 + 2.*f11*f23*f32) + D*(1.*f13*f21*f22 + 1.*f12*f21*f23 - 2.*f11*f22*f23)*f33 + C*(0.75*f23 + (2.*f11*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(1,7) = -1.*D*(f13*f21 - 1.*f11*f23)*(-1.*f23*f31 + f21*f33) + (2.*C*f12*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(2,7) = D*(1.*f11*f22*f23*f31 + std::pow(f21,2)*(2.*f13*f32 - 1.*f12*f33) + f21*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 - 2.*f11*f23*f32 + 1.*f11*f22*f33)) + C*(-0.75*f21 + (2.*f13*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(3,7) = D*f13*(-1. - 1.*f13*f22*f31 + 1.*f12*f23*f31 + 2.*f13*f21*f32 - 2.*f11*f23*f32) + D*(-2.*f12*f13*f21 + 1.*f11*f13*f22 + 1.*f11*f12*f23)*f33 + C*(-0.75*f13 + (2.*f21*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(4,7) = -1.*D*(f13*f21 - 1.*f11*f23)*(f13*f31 - 1.*f11*f33) + (2.*C*f22*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(5,7) = 1.*D*f12*f13*f21*f31 + D*f11*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 - 2.*f13*f21*f32 + 2.*f11*f23*f32 + 1.*f12*f21*f33 - 1.*f11*f22*f33) + C*(0.75*f11 + (2.*f23*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(6,7) = -1.*D*(f13*f21 - 1.*f11*f23)*(f13*f22 - 1.*f12*f23) + (2.*C*f31*f32)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(7,7) = 1.*C + 1.*D*std::pow(f13*f21 - 1.*f11*f23,2) + (2.*C*std::pow(f32,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));

            ddw(8,7) = -1.*D*(f12*f21 - 1.*f11*f22)*(f13*f21 - 1.*f11*f23) + (2.*C*f32*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(0,8) = D*(1.*f12*f21*f23*f32 + std::pow(f22,2)*(-1.*f13*f31 + 2.*f11*f33) + f22*(-1. + 1.*f12*f23*f31 + 1.*f13*f21*f32 - 2.*f11*f23*f32 - 2.*f12*f21*f33)) + C*(-0.75*f22 + (2.*f11*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(1,8) = D*(1.*f11*f22*f23*f31 + std::pow(f21,2)*(-1.*f13*f32 + 2.*f12*f33) + f21*(1. + 1.*f13*f22*f31 - 2.*f12*f23*f31 + 1.*f11*f23*f32 - 2.*f11*f22*f33)) + C*(0.75*f21 + (2.*f12*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(2,8) = -1.*D*(f12*f21 - 1.*f11*f22)*(-1.*f22*f31 + f21*f32) + (2.*C*f13*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(3,8) = D*(1.*f11*f13*f22*f32 + std::pow(f12,2)*(-1.*f23*f31 + 2.*f21*f33) + f12*(1. + 1.*f13*f22*f31 - 2.*f13*f21*f32 + 1.*f11*f23*f32 - 2.*f11*f22*f33)) + C*(0.75*f12 + (2.*f21*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(4,8) = 1.*D*f12*f13*f21*f31 + D*f11*(-1. - 2.*f13*f22*f31 + 1.*f12*f23*f31 + 1.*f13*f21*f32 - 1.*f11*f23*f32 - 2.*f12*f21*f33 + 2.*f11*f22*f33) + C*(-0.75*f11 + (2.*f22*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2));

            ddw(5,8) = -1.*D*(f12*f21 - 1.*f11*f22)*(f12*f31 - 1.*f11*f32) + (2.*C*f23*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(6,8) = -1.*D*(f12*f21 - 1.*f11*f22)*(-1.*f13*f22 + f12*f23) + (2.*C*f31*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(7,8) = -1.*D*(f12*f21 - 1.*f11*f22)*(f13*f21 - 1.*f11*f23) + (2.*C*f32*f33)/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2);

            ddw(8,8) = 1.*C + 1.*D*std::pow(f12*f21 - 1.*f11*f22,2) + (2.*C*std::pow(f33,2))/std::pow(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2),2) - (1.*C)/(1 + std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2));
        
        //muscle hessian
        ddw.block(0,0,3,1) += (fibre_direction*v1)*muscle_magnitude;
        ddw.block(0,1,3,1) += (fibre_direction*v2)*muscle_magnitude;
        ddw.block(0,2,3,1) += (fibre_direction*v3)*muscle_magnitude;
        
        ddw.block(3,3,3,1) += (fibre_direction*v1)*muscle_magnitude;
        ddw.block(3,4,3,1) += (fibre_direction*v2)*muscle_magnitude;
        ddw.block(3,5,3,1) += (fibre_direction*v3)*muscle_magnitude;
        
        ddw.block(6,6,3,1) += (fibre_direction*v1)*muscle_magnitude;
        ddw.block(6,7,3,1) += (fibre_direction*v2)*muscle_magnitude;
        ddw.block(6,8,3,1) += (fibre_direction*v3)*muscle_magnitude;
        // //Regular NH
        //     C = 0.5*C;
        //     D = 0.5*D;
        //     ddw(0,0) = 2*D*std::pow(f23*f32 - f22*f33,2) + (2*C*(-12*f11*(f23*f32 - f22*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) + 5*std::pow(f23*f32 - f22*f33,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,0) = 2*D*(f23*f31 - f21*f33)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f23*f31 - f21*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,0) = 2*D*(-(f22*f31) + f21*f32)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 6*f13*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,0) = 2*D*(f13*f32 - f12*f33)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f32 - f12*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,0) = 2*D*(-(f13*f31) + f11*f33)*(-(f23*f32) + f22*f33) + 2*D*f33*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f11*(-(f13*f31) + f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f22*(-(f23*f32) + f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,0) = 2*D*(f12*f31 - f11*f32)*(-(f23*f32) + f22*f33) + 2*D*f32*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f12*f31) + f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f23*(f23*f32 - f22*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f32*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,0) = 2*D*(-(f13*f22) + f12*f23)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f13*f22 - f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 6*f31*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,0) = 2*D*(f13*f21 - f11*f23)*(-(f23*f32) + f22*f33) + 2*D*f23*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f13*f21) + f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f32*(f23*f32 - f22*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f23*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,0) = 2*D*(-(f12*f21) + f11*f22)*(-(f23*f32) + f22*f33) - 2*D*f22*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f12*f21) + f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f33*(-(f23*f32) + f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f22*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,1) = 2*D*(f23*f31 - f21*f33)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f23*f31 - f21*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,1) = 2*D*std::pow(f23*f31 - f21*f33,2) + (2*C*(9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) - 12*f12*(f23*f31 - f21*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*std::pow(f23*f31 - f21*f33,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,1) = 2*D*(-(f22*f31) + f21*f32)*(f23*f31 - f21*f33) + (2*C*(6*f13*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,1) = 2*D*(f13*f32 - f12*f33)*(f23*f31 - f21*f33) + 2*D*f33*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(6*f12*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f32 - f12*f33)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,1) = 2*D*(-(f13*f31) + f11*f33)*(f23*f31 - f21*f33) + (2*C*(6*f22*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,1) = 2*D*(f12*f31 - f11*f32)*(f23*f31 - f21*f33) - 2*D*f31*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f12*(f12*f31 - f11*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f23*(f23*f31 - f21*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f31*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,1) = 2*D*(-(f13*f22) + f12*f23)*(f23*f31 - f21*f33) + 2*D*f23*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(6*f31*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f12*(-(f13*f22) + f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f23*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,1) = 2*D*(f13*f21 - f11*f23)*(f23*f31 - f21*f33) + (2*C*(6*f12*(f13*f21 - f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f32*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,1) = 2*D*(f12*f21 - f11*f22)*(-(f23*f31) + f21*f33) + 2*D*f21*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f12*(f12*f21 - f11*f22)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f33*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f21 - f11*f22)*(-(f23*f31) + f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f21*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,2) = 2*D*(-(f22*f31) + f21*f32)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 6*f13*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,2) = 2*D*(-(f22*f31) + f21*f32)*(f23*f31 - f21*f33) + (2*C*(6*f13*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,2) = 2*D*std::pow(f22*f31 - f21*f32,2) + (2*C*(-12*f13*(f22*f31 - f21*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) + 5*std::pow(f22*f31 - f21*f32,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,2) = 2*D*(-(f22*f31) + f21*f32)*(f13*f32 - f12*f33) + 2*D*f32*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(6*f13*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f21*(-(f22*f31) + f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f32*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,2) = 2*D*(f22*f31 - f21*f32)*(f13*f31 - f11*f33) + 2*D*f31*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f22*(f22*f31 - f21*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f13*(f13*f31 - f11*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f22*f31 - f21*f32)*(f13*f31 - f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f31*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,2) = 2*D*(f12*f31 - f11*f32)*(-(f22*f31) + f21*f32) + (2*C*(6*f13*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,2) = 2*D*(f13*f22 - f12*f23)*(f22*f31 - f21*f32) + 2*D*f22*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f13*(f13*f22 - f12*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f31*(f22*f31 - f21*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f22 - f12*f23)*(f22*f31 - f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f22*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,2) = 2*D*(f13*f21 - f11*f23)*(-(f22*f31) + f21*f32) + 2*D*f21*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f13*(f13*f21 - f11*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f32*(-(f22*f31) + f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f21*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,2) = 2*D*(-(f12*f21) + f11*f22)*(-(f22*f31) + f21*f32) + (2*C*(-6*(f22*f31 - f21*f32)*f33*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f13*(f12*f21 - f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,3) = 2*D*(f13*f32 - f12*f33)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f32 - f12*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,3) = 2*D*(f13*f32 - f12*f33)*(f23*f31 - f21*f33) + 2*D*f33*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(6*f12*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f32 - f12*f33)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,3) = 2*D*(-(f22*f31) + f21*f32)*(f13*f32 - f12*f33) + 2*D*f32*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(6*f13*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f21*(-(f22*f31) + f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f22*f31) + f21*f32)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f32*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,3) = 2*D*std::pow(f13*f32 - f12*f33,2) + (2*C*(9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) - 12*f21*(f13*f32 - f12*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*std::pow(f13*f32 - f12*f33,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,3) = 2*D*(-(f13*f31) + f11*f33)*(f13*f32 - f12*f33) + (2*C*(6*f22*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,3) = 2*D*(f12*f31 - f11*f32)*(f13*f32 - f12*f33) + (2*C*(6*f21*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f31 - f11*f32)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,3) = 2*D*(-(f13*f22) + f12*f23)*(f13*f32 - f12*f33) + (2*C*(6*f31*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f13*f22 - f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,3) = 2*D*(f13*f21 - f11*f23)*(f13*f32 - f12*f33) - 2*D*f13*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f21*(f13*f21 - f11*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f32*(f13*f32 - f12*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f13*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,3) = 2*D*(f12*f21 - f11*f22)*(-(f13*f32) + f12*f33) + 2*D*f12*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f21*(f12*f21 - f11*f22)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f33*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f21 - f11*f22)*(-(f13*f32) + f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f12*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,4) = 2*D*(-(f13*f31) + f11*f33)*(-(f23*f32) + f22*f33) + 2*D*f33*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f11*(-(f13*f31) + f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f22*(-(f23*f32) + f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,4) = 2*D*(-(f13*f31) + f11*f33)*(f23*f31 - f21*f33) + (2*C*(6*f22*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f12*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,4) = 2*D*(f22*f31 - f21*f32)*(f13*f31 - f11*f33) + 2*D*f31*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f22*(f22*f31 - f21*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f13*(f13*f31 - f11*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f22*f31 - f21*f32)*(f13*f31 - f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f31*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,4) = 2*D*(-(f13*f31) + f11*f33)*(f13*f32 - f12*f33) + (2*C*(6*f22*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f31) + f11*f33)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,4) = 2*D*std::pow(f13*f31 - f11*f33,2) + (2*C*(-12*f22*(f13*f31 - f11*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) + 5*std::pow(f13*f31 - f11*f33,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,4) = 2*D*(f12*f31 - f11*f32)*(-(f13*f31) + f11*f33) + (2*C*(6*f22*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,4) = 2*D*(f13*f22 - f12*f23)*(f13*f31 - f11*f33) + 2*D*f13*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f22*(f13*f22 - f12*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f31*(f13*f31 - f11*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f22 - f12*f23)*(f13*f31 - f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f13*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,4) = 2*D*(f13*f21 - f11*f23)*(-(f13*f31) + f11*f33) + (2*C*(6*f22*(f13*f21 - f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f32*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,4) = 2*D*(-(f12*f21) + f11*f22)*(-(f13*f31) + f11*f33) + 2*D*f11*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f22*(-(f12*f21) + f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f33*(-(f13*f31) + f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f11*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,5) = 2*D*(f12*f31 - f11*f32)*(-(f23*f32) + f22*f33) + 2*D*f32*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f12*f31) + f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f23*(f23*f32 - f22*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f32*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,5) = 2*D*(f12*f31 - f11*f32)*(f23*f31 - f21*f33) - 2*D*f31*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f12*(f12*f31 - f11*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f23*(f23*f31 - f21*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f31*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,5) = 2*D*(f12*f31 - f11*f32)*(-(f22*f31) + f21*f32) + (2*C*(6*f13*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f22*f31 - f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,5) = 2*D*(f12*f31 - f11*f32)*(f13*f32 - f12*f33) + (2*C*(6*f21*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f31 - f11*f32)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,5) = 2*D*(f12*f31 - f11*f32)*(-(f13*f31) + f11*f33) + (2*C*(6*f22*(f12*f31 - f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f23*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f12*f31 - f11*f32)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,5) = 2*D*std::pow(f12*f31 - f11*f32,2) + (2*C*(9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) - 12*f23*(f12*f31 - f11*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*std::pow(f12*f31 - f11*f32,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,5) = 2*D*(-(f13*f22) + f12*f23)*(f12*f31 - f11*f32) + 2*D*f12*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f23*(-(f13*f22) + f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f31*(f12*f31 - f11*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f12*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,5) = 2*D*(f13*f21 - f11*f23)*(f12*f31 - f11*f32) + 2*D*f11*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f23*(-(f13*f21) + f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f32*(-(f12*f31) + f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f11*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,5) = 2*D*(-(f12*f21) + f11*f22)*(f12*f31 - f11*f32) + (2*C*(-6*(f12*f21 - f11*f22)*f23*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f12*f31 - f11*f32)*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,6) = 2*D*(-(f13*f22) + f12*f23)*(-(f23*f32) + f22*f33) + (2*C*(6*f11*(f13*f22 - f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 6*f31*(f23*f32 - f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,6) = 2*D*(-(f13*f22) + f12*f23)*(f23*f31 - f21*f33) + 2*D*f23*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(6*f31*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f12*(-(f13*f22) + f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f23*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,6) = 2*D*(f13*f22 - f12*f23)*(f22*f31 - f21*f32) + 2*D*f22*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f13*(f13*f22 - f12*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f31*(f22*f31 - f21*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f22 - f12*f23)*(f22*f31 - f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f22*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,6) = 2*D*(-(f13*f22) + f12*f23)*(f13*f32 - f12*f33) + (2*C*(6*f31*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f21*(f13*f22 - f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,6) = 2*D*(f13*f22 - f12*f23)*(f13*f31 - f11*f33) + 2*D*f13*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f22*(f13*f22 - f12*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f31*(f13*f31 - f11*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f22 - f12*f23)*(f13*f31 - f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f13*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,6) = 2*D*(-(f13*f22) + f12*f23)*(f12*f31 - f11*f32) + 2*D*f12*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f23*(-(f13*f22) + f12*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f31*(f12*f31 - f11*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f13*f22) + f12*f23)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f12*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,6) = 2*D*std::pow(f13*f22 - f12*f23,2) + (2*C*(-12*(f13*f22 - f12*f23)*f31*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) + 5*std::pow(f13*f22 - f12*f23,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,6) = 2*D*(f13*f21 - f11*f23)*(-(f13*f22) + f12*f23) + (2*C*(-6*(f13*f22 - f12*f23)*f32*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f21 - f11*f23)*f31*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f13*f22) + f12*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,6) = 2*D*(-(f12*f21) + f11*f22)*(-(f13*f22) + f12*f23) + (2*C*(-6*(f12*f21 - f11*f22)*f31*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f22 - f12*f23)*f33*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f13*f22) + f12*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,7) = 2*D*(f13*f21 - f11*f23)*(-(f23*f32) + f22*f33) + 2*D*f23*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f13*f21) + f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f32*(f23*f32 - f22*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f23*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,7) = 2*D*(f13*f21 - f11*f23)*(f23*f31 - f21*f33) + (2*C*(6*f12*(f13*f21 - f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f32*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f23*f31 - f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,7) = 2*D*(f13*f21 - f11*f23)*(-(f22*f31) + f21*f32) + 2*D*f21*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f13*(f13*f21 - f11*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f32*(-(f22*f31) + f21*f32)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f21*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,7) = 2*D*(f13*f21 - f11*f23)*(f13*f32 - f12*f33) - 2*D*f13*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f21*(f13*f21 - f11*f23)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f32*(f13*f32 - f12*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f13*f32 - f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f13*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,7) = 2*D*(f13*f21 - f11*f23)*(-(f13*f31) + f11*f33) + (2*C*(6*f22*(f13*f21 - f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f32*(f13*f31 - f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,7) = 2*D*(f13*f21 - f11*f23)*(f12*f31 - f11*f32) + 2*D*f11*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f23*(-(f13*f21) + f11*f23)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*f32*(-(f12*f31) + f11*f32)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f13*f21 - f11*f23)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f11*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,7) = 2*D*(f13*f21 - f11*f23)*(-(f13*f22) + f12*f23) + (2*C*(-6*(f13*f22 - f12*f23)*f32*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f21 - f11*f23)*f31*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(f13*f21 - f11*f23)*(-(f13*f22) + f12*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,7) = 2*D*std::pow(f13*f21 - f11*f23,2) + (2*C*(9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) - 12*(f13*f21 - f11*f23)*f32*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*std::pow(f13*f21 - f11*f23,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,7) = 2*D*(-(f12*f21) + f11*f22)*(f13*f21 - f11*f23) + (2*C*(-6*(f12*f21 - f11*f22)*f32*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f21 - f11*f23)*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(f13*f21 - f11*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(0,8) = 2*D*(-(f12*f21) + f11*f22)*(-(f23*f32) + f22*f33) - 2*D*f22*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f11*(-(f12*f21) + f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f33*(-(f23*f32) + f22*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f23*f32) + f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f22*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(1,8) = 2*D*(f12*f21 - f11*f22)*(-(f23*f31) + f21*f33) + 2*D*f21*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f12*(f12*f21 - f11*f22)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f33*(f23*f31 - f21*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f21 - f11*f22)*(-(f23*f31) + f21*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f21*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(2,8) = 2*D*(-(f12*f21) + f11*f22)*(-(f22*f31) + f21*f32) + (2*C*(-6*(f22*f31 - f21*f32)*f33*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f13*(f12*f21 - f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f22*f31) + f21*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(3,8) = 2*D*(f12*f21 - f11*f22)*(-(f13*f32) + f12*f33) + 2*D*f12*(1 + f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + (2*C*(-6*f21*(f12*f21 - f11*f22)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 6*f33*(f13*f32 - f12*f33)*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(f12*f21 - f11*f22)*(-(f13*f32) + f12*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) + 3*f12*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(4,8) = 2*D*(-(f12*f21) + f11*f22)*(-(f13*f31) + f11*f33) + 2*D*f11*(-1 - f13*f22*f31 + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + (2*C*(-6*f22*(-(f12*f21) + f11*f22)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) - 6*f33*(-(f13*f31) + f11*f33)*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f13*f31) + f11*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2)) - 3*f11*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(5,8) = 2*D*(-(f12*f21) + f11*f22)*(f12*f31 - f11*f32) + (2*C*(-6*(f12*f21 - f11*f22)*f23*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f12*f31 - f11*f32)*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(f12*f31 - f11*f32)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(6,8) = 2*D*(-(f12*f21) + f11*f22)*(-(f13*f22) + f12*f23) + (2*C*(-6*(f12*f21 - f11*f22)*f31*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f22 - f12*f23)*f33*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(-(f13*f22) + f12*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(7,8) = 2*D*(-(f12*f21) + f11*f22)*(f13*f21 - f11*f23) + (2*C*(-6*(f12*f21 - f11*f22)*f32*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) - 6*(f13*f21 - f11*f23)*f33*(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33) + 5*(-(f12*f21) + f11*f22)*(f13*f21 - f11*f23)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            
        //     ddw(8,8) = 2*D*std::pow(f12*f21 - f11*f22,2) + (2*C*(-12*(f12*f21 - f11*f22)*f33*(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33) + 9*std::pow(f13*f22*f31 - f12*f23*f31 - f13*f21*f32 + f11*f23*f32 + f12*f21*f33 - f11*f22*f33,2) + 5*std::pow(f12*f21 - f11*f22,2)*(std::pow(f11,2) + std::pow(f12,2) + std::pow(f13,2) + std::pow(f21,2) + std::pow(f22,2) + std::pow(f23,2) + std::pow(f31,2) + std::pow(f32,2) + std::pow(f33,2))))/(9.*musclePow(-(f13*f22*f31) + f12*f23*f31 + f13*f21*f32 - f11*f23*f32 - f12*f21*f33 + f11*f22*f33,8.0));
            


        H = -gradX.transpose()*ddw.block(0,0,3,3)*gradX +
            -gradX.transpose()*ddw.block(0,3,3,3)*gradY +
            -gradX.transpose()*ddw.block(0,6,3,3)*gradZ +
            -gradY.transpose()*ddw.block(3,0,3,3)*gradX +
            -gradY.transpose()*ddw.block(3,3,3,3)*gradY +
            -gradY.transpose()*ddw.block(3,6,3,3)*gradZ +
            -gradZ.transpose()*ddw.block(6,0,3,3)*gradX +
            -gradZ.transpose()*ddw.block(6,3,3,3)*gradY +
            -gradZ.transpose()*ddw.block(6,6,3,3)*gradZ;
        
        
        // hard coded for tet, need to change size for hex
       /*Eigen::SelfAdjointEigenSolver<Matrix> es(-H);

        Eigen::MatrixXd DiagEval = es.eigenvalues().real().asDiagonal();
        Eigen::MatrixXd Evec = es.eigenvectors().real();

        for (int i = 0; i < 12; ++i) {
            if (es.eigenvalues()[i]<0) {
                DiagEval(i,i) = 1e-8;
            }
        }
                saveMarket(H, "H.dat");
        H = -Evec * DiagEval * Evec.transpose();*/
        
        
    }
    
    template<typename Matrix>
    inline void getCauchyStress(Matrix &S, double *x, State<DataType> &state) {
        double mu = 2*m_C;
        double lambda = 2*m_D;

        Eigen::Matrix<DataType, 3,3> F = ShapeFunction::F(x,state) + Eigen::Matrix<DataType,3,3>::Identity();
        double J = F.determinant();
        auto F_inv_T = F.inverse().transpose();

        auto P = mu * (F - F_inv_T) + lambda * log(J) * F_inv_T;

        S = (P * F.transpose()) / J;
    }

    
    inline const DataType & getC() const { return m_C; }
    inline const DataType & getD() const { return m_D; }
    
    inline const DataType & getE() const { return m_C; }

protected:
    DataType m_C, m_D;
    Eigen::Vector3d fibre_direction;
    double muscle_magnitude;
private:
    
};

#endif
