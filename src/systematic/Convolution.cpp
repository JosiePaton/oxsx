#include <Convolution.h>
#include <PDF.h>
#include <Exceptions.h>
#include <JumpDist.h>
#include <string>
void 
Convolution::SetFunction(PDF* function_){
    // wrap this up if position independent kernal of the form P(x | x2) = P(x - x2)
    fDist = static_cast<ConditDist*>(new JumpDist(function_));
}

void
Convolution::SetConditDist(ConditDist* c_){
    fDist = c_->Clone();
}

Convolution::~Convolution(){
    delete fDist;
}

void 
Convolution::SetAxes(const AxisCollection& axes_){
    fPdfMapping.SetAxes(axes_);
    fHasAxes = true;
}

void 
Convolution::Construct(){
    if (!fDist || !fHasAxes)
        throw LogicError("Convolution::Construct() : Tried to construct convolution without axes or function/distribution, or both!!");
    
    if(!fCachedCompatibleBins)
        CacheCompatibleBins();

    size_t nBins = fPdfMapping.GetNBins();
    size_t nDims = fPdfMapping.GetAxes().GetNDimensions();
    const AxisCollection& axes = fPdfMapping.GetAxes();
    std::vector<size_t> relativeIndices = fDataRep.GetRelativeIndices(fPdfDataRep);

    // Work out the transition probabilitites within this sub set of the bins
    std::vector<double> binCentres(fSysAxes.GetNDimensions());
    std::vector<double> lowEdges(fSysAxes.GetNDimensions());
    std::vector<double> highEdges(fSysAxes.GetNDimensions());

    PdfMapping subMap;
    subMap.SetAxes(fSysAxes);

    for (size_t origBin = 0; origBin < fSysAxes.GetNBins(); origBin++){
        // get the centre of the bin. Need to offset by this for a convolution
        fSysAxes.GetBinCentres(origBin, binCentres);

        // loop over the bins it can be smeared into 
        for(size_t destBin = 0; destBin < fSysAxes.GetNBins(); destBin++){
            fSysAxes.GetBinLowEdges(destBin, lowEdges);
            fSysAxes.GetBinHighEdges(destBin, highEdges);
            
			subMap.SetComponent(destBin, origBin, fDist -> Integral(lowEdges, highEdges, binCentres));
        }        
    }

    // Now expand to the full size matrix. Elements are zero by default
    // compatible bins are cached, values must match the smaller matrix above
    size_t destBin = -1;
    std::vector<unsigned> nonZeroRowIndices;
    std::vector<unsigned> nonZeroColIndices;
    std::vector<double> values;
    nonZeroRowIndices.reserve(fCompatibleBins.at(0).size());
    nonZeroColIndices.reserve(fCompatibleBins.at(0).size());
    

    for(size_t origBin = 0; origBin < axes.GetNBins(); origBin++){
        for(size_t i = 0; i < fCompatibleBins.at(origBin).size(); i++){
            destBin = fCompatibleBins.at(origBin).at(i);
            nonZeroRowIndices.push_back(origBin);
            nonZeroColIndices.push_back(destBin);
            values.push_back( subMap.GetComponent(fSysBins.at(origBin),
                                                  fSysBins.at(destBin)));
        }
    }
        
    fPdfMapping.SetComponents(nonZeroRowIndices, nonZeroColIndices, values);
}


void
Convolution::CacheCompatibleBins(){
    fCompatibleBins.resize(fPdfMapping.GetNBins());
    // only need to look at one side of the matrix, its symmetric
    for(size_t i = 0; i < fPdfMapping.GetNBins(); i++){
        fCompatibleBins.at(i).push_back(i); // always true
        for(size_t j = i+1;  j < fPdfMapping.GetNBins(); j++){
            if(BinsCompatible(i , j)){
                fCompatibleBins.at(i).push_back(j);
                fCompatibleBins.at(j).push_back(i);
            }
        }
    }

    std::vector<size_t> relativeIndices = fDataRep.GetRelativeIndices(fPdfDataRep);
    const AxisCollection& axes = fPdfMapping.GetAxes();

    //  get the axes that this systematic will act on
    fSysAxes = AxisCollection();
    for(size_t i = 0; i < relativeIndices.size(); i++)
      fSysAxes.AddAxis(axes.GetAxis(relativeIndices.at(i)));
    
    // cache the equivilent index in the binning system of the systematic
    fSysBins.resize(fPdfMapping.GetNBins());
    std::vector<size_t> sysIndices(relativeIndices.size(), 0);
    for(size_t i = 0; i < axes.GetNBins(); i++){
      for(size_t dim = 0; dim < relativeIndices.size(); dim++)
          sysIndices[dim] = axes.UnflattenIndex(i, relativeIndices.at(dim));

      fSysBins[i] = fSysAxes.FlattenIndices(sysIndices);
    }
    fCachedCompatibleBins = true;
}

///////////////////////////////
// Make this object fittable //
///////////////////////////////

void
Convolution::MakeFittable(){
    fDist->MakeFittable();
}

std::vector<std::string>
Convolution::GetParameterNames() const{
    std::vector<std::string> param = fDist->GetParameterNames();
    for(size_t i = 0; i < param.size(); i++){
        param[i] = "Convolution : " + param[i];        
    }
    return param;
}

std::vector<double>
Convolution::GetParameters() const{
    return fDist->GetParameters();
}

size_t
Convolution::GetParameterCount() const{
    return fDist->GetParameterCount();
}

void
Convolution::SetParameters(const std::vector<double>& params_){
    try{
        fDist->SetParameters(params_);
    }
    catch(const ParameterCountError& e){
        throw ParameterCountError(std::string("Convolution : ") 
                                  + e.what());
    }
}
