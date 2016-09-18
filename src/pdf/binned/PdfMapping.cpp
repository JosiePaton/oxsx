 #include <PdfMapping.h>
#include <BinnedPhysDist.h>
#include <iostream>
#include <Exceptions.h>

// Initialise the detector response to zero
void 
PdfMapping::SetAxes(const AxisCollection& axes_){
    fAxes  = axes_;
    fNBins = axes_.GetNBins();
    fNDims = axes_.GetNDimensions();
    fResponse = arma::sp_mat(fNBins, fNBins);
}


const AxisCollection& 
PdfMapping::GetAxes() const{
    return fAxes;
}

void 
PdfMapping::SetResponse(const arma::sp_mat& response_){
    fResponse = response_;
}

void 
PdfMapping::SetComponent(size_t col_, size_t row_, double val_){
    if (col_ >= fNBins || row_ >= fNBins)
        throw NotFoundError(Formatter() << "Attempted out of bounds access on response matrix (" << row_ <<  "," << col_ << "). Is it initialised with axes?");

    fResponse(col_,row_) = val_; 
}

double 
PdfMapping::GetComponent(size_t col_, size_t row_) const{
    if (col_ >= fNBins || row_ >= fNBins)
        throw NotFoundError(Formatter() << "Attempted out of bounds access on response matrix (" << row_ <<  "," << col_ << "). Is it initialised with axes?");

    return fResponse(col_, row_);
}

BinnedPhysDist
PdfMapping::operator() (const BinnedPhysDist& pdf_) const{
    if (!fNDims){
        throw DimensionError("PdfMapping::operator() :  NDims = 0, have you set the axes?");
    }
    
    if(pdf_.GetNDims() < fNDims){
        throw DimensionError(Formatter() << "PdfMapping::opeator() : PDF Dimensionality ("
                                         << pdf_.GetNDims() << ")" 
                                         << " too small for PdfMap ("
                                         << fNDims
                                         << " ) to act on");
    }


    BinnedPhysDist observedPdf(pdf_.GetAxes());
    observedPdf.SetDataRep(pdf_.GetDataRep());
    
    arma::vec newContents;
    try{
        // convert to armadillo vec
        newContents = fResponse * arma::vec(pdf_.GetBinContents());
    }
    catch(const std::logic_error& e_){
        throw DimensionError(Formatter() << "PdfMapping::operator() : matrix multiplation failed. Pdf has "
                                         << pdf_.GetNBins() << " bins, but matrix built for " << fAxes.GetNBins()
                             );
    }

    // armadillo function for quick transfer to std::vector double
    observedPdf.SetBinContents(arma::conv_to<std::vector<double> >::from((newContents)));
              
    return observedPdf;
}

PdfMapping
PdfMapping::operator*=(const PdfMapping& other_){
  fResponse = fResponse * other_.fResponse;
  return *this;
}

void
PdfMapping::SetZeros(){
    if(!fNBins)
        return;
    fResponse = arma::sp_mat(fNBins, fNBins);
}

// FIXME: unsigned vs. size_t
void 
PdfMapping::SetComponents(const std::vector<unsigned>& rowIndices_,
                          const std::vector<unsigned>& colIndices_,
                          const std::vector<double>& values_){
    if(rowIndices_.size() != values_.size() || colIndices_.size() != values_.size())
        throw DimensionError("PdfMapping::SetComponent() #values != #locations");

    arma::umat locs(2, rowIndices_.size());
    locs.row(0) = arma::urowvec(rowIndices_);
    locs.row(1) = arma::urowvec(colIndices_);

    fResponse = arma::sp_mat(locs, arma::vec(values_));
}
