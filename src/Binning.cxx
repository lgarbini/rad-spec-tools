// Copyright (C) 2013 Oliver Schulz <oliver.schulz@tu-dortmund.de>

// This is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#include "Binning.h"

#include <iostream>


using namespace std;


namespace rspt {


Binning::Binning(const TAxis *axis) {
	Int_t fromBin = axis->GetFirst();
	Int_t toBin = axis->GetLast();
	Double_t from = axis->GetBinLowEdge(fromBin);
	Double_t to = axis->GetBinLowEdge(toBin);
	Double_t bw = (from - to) / (fromBin - toBin);
	
	m_from = from;
	m_binWidth = bw;
	m_nBins = toBin - fromBin + 1;
}


} // namespace rspt
