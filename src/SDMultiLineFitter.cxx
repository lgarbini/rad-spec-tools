/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2014  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "SDMultiLineFitter.h"

#include<memory>

#include "rsptutils.h"

#include "HistAnalysis.h"
#include <TSpectrum.h>
namespace rspt{
SDMultiLineFitter::SDMultiLineFitter()
{
	m_preCalibration_ch2e=NULL;
	m_preCalibration_e2ch=NULL;
	debug=true;
	init();
}

SDMultiLineFitter::~SDMultiLineFitter()
{

}

void SDMultiLineFitter::init()
{
	if(m_preCalibration_ch2e!=0||m_preCalibration_e2ch!=0){
		resetPreCal();
	}
}

void SDMultiLineFitter::setRange(double lowEdge, double highEdge)
{
	if(lowEdge<highEdge) {
		m_low_limit=lowEdge;
		m_high_limit=highEdge;
	} else {
		m_low_limit=0;
		m_high_limit=0;
		std::cerr<<"lower Limit > higherLimit. Reset to full range."<<std::endl;
	}
}

void SDMultiLineFitter::setPreCal(double slope, double intercept)
{
	m_preCalibration_ch2e=new TF1("preCal", "pol1", 0, m_maxADCch);
	m_preCalibration_ch2e->SetParameter(1,slope);
	m_preCalibration_ch2e->SetParameter(0,intercept);
	m_preCalibration_e2ch=(TF1*)m_preCalibration_ch2e->Clone();
	transposePol1(&m_preCalibration_e2ch);
}

void SDMultiLineFitter::setPreCal(TF1* precal_ch2e)
{
	if(precal_ch2e!=0){
		m_preCalibration_ch2e=dynamic_cast<TF1*>(precal_ch2e->Clone());
		m_preCalibration_e2ch=dynamic_cast<TF1*>(m_preCalibration_ch2e->Clone());
		transposePol1(&m_preCalibration_e2ch);
	}else{
		std::cerr<<"Provided precalibration function is invalid: NULL ptr"<<std::endl;
	}
}

void SDMultiLineFitter::resetPreCal()
{
	if(m_preCalibration_ch2e!=0) {
		delete m_preCalibration_ch2e;
		m_preCalibration_ch2e=NULL;
	}
	if(m_preCalibration_e2ch!=0) {
		delete m_preCalibration_e2ch;
		m_preCalibration_e2ch=NULL;
	}
}

void SDMultiLineFitter::setSigma(float sig)
{
	if(sig>0) {
		m_sigma=sig;
	} else {
		std::cerr<<"sigma must be greater than zero"<<std::endl;
	}
}

void SDMultiLineFitter::setThreshold(double thresh)
{
	if(thresh>0) {
		m_threshold=thresh;
	} else {
		std::cerr<<"threshold must be greater than zero"<<std::endl;
	}
}

std::pair< double,int> SDMultiLineFitter::getRange(std::vector<double> energy,int iter,int lines_to_fit)
{
	double fitrange;
	double extended_fitrange;
	int fitted_lines;
	fitrange=m_width*m_preCalibration_e2ch->Eval(energy[iter]);
	extended_fitrange=fitrange;
	fitted_lines=1;
	if(iter<lines_to_fit-1) {
		while(true) {
			if(iter!=lines_to_fit-1) {
				if(m_preCalibration_e2ch->Eval(energy[iter])+extended_fitrange<m_preCalibration_e2ch->Eval(energy[iter+fitted_lines])) {
					if ( debug ) std::cerr<<"e2ch(E["<<iter<<"])+0.05*e2ch(E["<<iter<<"]) < e2ch(E["<<iter+fitted_lines<<"])" <<std::endl;
				break;
				}
				extended_fitrange=m_preCalibration_e2ch->Eval(energy[iter+fitted_lines])+fitrange-m_preCalibration_e2ch->Eval(energy[iter]);
				fitted_lines++;
			}
			else {
				break;
				}
		}
	}
	return std::make_pair(extended_fitrange,fitted_lines);
}


std::vector<rspt::SDFitData*> SDMultiLineFitter::makeCalFits(TH1* raw_hist, std::vector<double> energy, double s_factor, std::vector<bool> *reject_res_cal)
{
	std::vector<rspt::SDFitData*> fits;
	if (raw_hist==NULL){
		std::cerr<<"raw_hist ptr is invalid"<<std::endl;
		return fits;
	}
	if (m_preCalibration_ch2e==0){
		std::cerr<<"does not find a precalibration aborting"<<std::endl;
		return fits;
	}

	int npeaks=energy.size();
	if ( m_low_limit<m_high_limit ){
		raw_hist->GetXaxis()->SetRangeUser(m_low_limit,m_high_limit);
	}

	TSpectrum *spec=new TSpectrum();
	spec->SetResolution(m_sigma);

	int lines_to_fit=npeaks;
	std::pair<double,int> range_info;
	rspt::SDFitData *result;

	if ( debug ) for (unsigned int i= 0; i<npeaks; ++i ) std::cerr<<"energy["<<i<<"] = "<<energy[i]<<"\t ADC channel["<<i<<"] = "<<m_preCalibration_e2ch->Eval(energy[i])<<std::endl;
	
	for ( unsigned int i=0; i<npeaks; i++ ){

		double fitrange=m_width*m_preCalibration_e2ch->Eval(energy[i]);
		range_info=getRange(energy,i,lines_to_fit);
		lines_to_fit-=range_info.second;
		raw_hist->GetXaxis()->SetRangeUser(m_preCalibration_e2ch->Eval(energy[i])-fitrange,m_preCalibration_e2ch->Eval(energy[i])+range_info.first);

		if (debug) {
			std::cerr<<" "<<std::endl;
			std::cerr<<" ********* i = "<<i<<" *********"<<std::endl;
			std::cerr<<"e2ch(E["<<i<<"]) = "<<m_preCalibration_e2ch->Eval(energy[i])<<"\t extended_fitrange = "<<range_info.first<<"\t fitted_lines = "<<range_info.second<<std::endl;
			std::cerr<<"lines_to_fit = "<<lines_to_fit<<std::endl;
			std::cerr<<"raw_hist X axis Range User = "<<m_preCalibration_e2ch->Eval(energy[i])-fitrange<<", "<<m_preCalibration_e2ch->Eval(energy[i])+range_info.first<<std::endl;
		}

		TSpectrum *spec = rspt::HistAnalysis::findPeaks(raw_hist, "", s_factor*energy[i], m_threshold);
		TF1 *fit = rspt::HistAnalysis::fitPeaks( raw_hist, spec, "+Q", "", false, "pol1", s_factor*energy[i]);
		
		int n_tspec_peaks = spec->GetNPeaks();
		fit->ResetBit(512);
		
		if(fit!=0){
			result=new rspt::SDFitData( fit, n_tspec_peaks );
			rspt::desiredPeak( i, range_info.second, energy, result, m_preCalibration_ch2e );
			fits.push_back(result);
            	}
		else std::cerr<<"fit failed! continue"<<std::endl;
        
		if(reject_res_cal!=0){
			if(reject_res_cal->size()==energy.size()){
				for(int curr_line=i;curr_line<(i+range_info.second);++curr_line){
					if(reject_res_cal->at(curr_line)==true) result->setResUsage(energy[curr_line]);
				}
			}
		}
		if(range_info.second>1) i+=range_info.second-1;
	}
	return fits;
}




}//namespace rspt