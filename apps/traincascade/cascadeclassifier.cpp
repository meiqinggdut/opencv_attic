#include "cascadeclassifier.h"
#include <queue>

using namespace std;

static const char* stageTypes[] = { CC_BOOST };
static const char* featureTypes[] = { CC_HAAR, CC_LBP };

CvCascadeParams::CvCascadeParams() : stageType( defaultStageType ), 
    featureType( defaultFeatureType ), winSize( cvSize(24, 24) )
{ 
    name = CC_CASCADE_PARAMS; 
}
CvCascadeParams::CvCascadeParams( int _stageType, int _featureType ) : stageType( _stageType ),
    featureType( _featureType ), winSize( cvSize(24, 24) )
{ 
    name = CC_CASCADE_PARAMS;
}

//---------------------------- CascadeParams --------------------------------------

void CvCascadeParams::write( FileStorage &fs ) const
{
    String stageTypeStr = stageType == BOOST ? CC_BOOST : String();
    CV_Assert( !stageTypeStr.empty() );
    fs << CC_STAGE_TYPE << stageTypeStr;
    String featureTypeStr = featureType == CvFeatureParams::HAAR ? CC_HAAR :
                            featureType == CvFeatureParams::LBP ? CC_LBP : 0;
    CV_Assert( !stageTypeStr.empty() );
    fs << CC_FEATURE_TYPE << featureTypeStr;
    fs << CC_HEIGHT << winSize.height;
    fs << CC_WIDTH << winSize.width;
}

bool CvCascadeParams::read( const FileNode &node )
{
    if ( node.empty() )
        return false;
    String stageTypeStr, featureTypeStr;
    FileNode rnode = node[CC_STAGE_TYPE];
    if ( !rnode.isString() )
        return false;
    rnode >> stageTypeStr;
    stageType = !stageTypeStr.compare( CC_BOOST ) ? BOOST : -1;
    if (stageType == -1)
        return false;
    rnode = node[CC_FEATURE_TYPE];
    if ( !rnode.isString() )
        return false;
    rnode >> featureTypeStr;
    featureType = !featureTypeStr.compare( CC_HAAR ) ? CvFeatureParams::HAAR :
                  !featureTypeStr.compare( CC_LBP ) ? CvFeatureParams::LBP : -1;
    if (featureType == -1)
        return false;
    node[CC_HEIGHT] >> winSize.height;
    node[CC_WIDTH] >> winSize.width;
    return winSize.height > 0 && winSize.width > 0;
}

void CvCascadeParams::printDefaults() const
{
    CvParams::printDefaults();
    cout << "  [-stageType <";
    for( int i = 0; i < (int)(sizeof(stageTypes)/sizeof(stageTypes[0])); i++ )
    {
        cout << (i ? " | " : "") << stageTypes[i];
        if ( i == defaultStageType )
            cout << "(default)";
    }
    cout << ">]" << endl;

    cout << "  [-featureType <{";
    for( int i = 0; i < (int)(sizeof(featureTypes)/sizeof(featureTypes[0])); i++ )
    {
        cout << (i ? ", " : "") << featureTypes[i];
        if ( i == defaultStageType )
            cout << "(default)";
    }
    cout << "}>]" << endl;
    cout << "  [-w <sampleWidth = " << winSize.width << ">]" << endl;
    cout << "  [-h <sampleHeight = " << winSize.height << ">]" << endl;
}

void CvCascadeParams::printAttrs() const
{
    cout << "stageType: " << stageTypes[stageType] << endl;
    cout << "featureType: " << featureTypes[featureType] << endl;
    cout << "sampleWidth: " << winSize.width << endl;
    cout << "sampleHeight: " << winSize.height << endl;
}

bool CvCascadeParams::scanAttr( const String prmName, const String val )
{
    bool res = true;
    if( !prmName.compare( "-stageType" ) )
    {
        for( int i = 0; i < (int)(sizeof(stageTypes)/sizeof(stageTypes[0])); i++ )
            if( !val.compare( stageTypes[i] ) )
                stageType = i;
    }
    else if( !prmName.compare( "-featureType" ) )
    {
        for( int i = 0; i < (int)(sizeof(featureTypes)/sizeof(featureTypes[0])); i++ )
            if( !val.compare( featureTypes[i] ) )
                featureType = i;
    }
    else if( !prmName.compare( "-w" ) )
    {
        winSize.width = atoi( val.c_str() );
    }
    else if( !prmName.compare( "-h" ) )
    {
        winSize.height = atoi( val.c_str() );
    }
    else
        res = false;
    return res;
}

//---------------------------- CascadeClassifier --------------------------------------

bool CvCascadeClassifier::train( const String _cascadeDirName,
                                const String _posFilename,
                                const String _negFilename, 
                                int _numPos, int _numNeg, 
                                int _numPrecalcVal, int _numPrecalcIdx,
                                int _numStages,
                                const CvCascadeParams& _cascadeParams,
                                const CvFeatureParams& _featureParams,
                                const CvCascadeBoostParams& _stageParams,
                                bool baseFormatSave )
{   
    if( _cascadeDirName.empty() || _posFilename.empty() || _negFilename.empty() )
        CV_Error( CV_StsBadArg, "_cascadeDirName or _bgfileName or _vecFileName is NULL" );

    String dirName;
    if ( _cascadeDirName.find('/') )
        dirName = _cascadeDirName + '/';
    else
        dirName = _cascadeDirName + '\\';

    numPos = _numPos;
    numNeg = _numNeg;
    numStages = _numStages;
    imgReader.create( _posFilename, _negFilename, cascadeParams.winSize );
    if ( !load( dirName ) )
    {
        cascadeParams = _cascadeParams;
        featureParams = CvFeatureParams::create(cascadeParams.featureType);
        featureParams->init(_featureParams);
        stageParams = new CvCascadeBoostParams;
        *stageParams = _stageParams;
        featureEvaluator = CvFeatureEvaluator::create(cascadeParams.featureType);
        featureEvaluator->init( (CvFeatureParams*)featureParams, numPos + numNeg, cascadeParams.winSize );
        stageClassifiers.reserve( numStages );
    }
    cout << "PARAMETERS:" << endl;
    cout << "cascadeDirName: " << _cascadeDirName << endl;
    cout << "vecFileName: " << _posFilename << endl;
    cout << "bgFileName: " << _negFilename << endl;
    cout << "numPos: " << _numPos << endl;
    cout << "numNeg: " << _numNeg << endl;
    cout << "numStages: " << numStages << endl;
    cout << "numPrecalcValues : " << _numPrecalcVal << endl;
    cout << "numPrecalcIndices : " << _numPrecalcIdx << endl;
    cascadeParams.printAttrs();
    stageParams->printAttrs();
    featureParams->printAttrs();

    int startNumStages = (int)stageClassifiers.size();
    if ( startNumStages > 1 )
        cout << endl << "Stages 0-" << startNumStages-1 << " are loaded" << endl;
    else if ( startNumStages == 1)
        cout << endl << "Stage 0 is loaded" << endl;
    
    double requiredLeafFARate = pow( (double) stageParams->maxFalseAlarm, (double) numStages ) /
                                (double)stageParams->max_depth;
    double tempLeafFARate;
    
    for( int i = startNumStages; i < numStages; i++ )
    {
        cout << endl << "===== TRAINING " << i << "-stage =====" << endl;
        cout << "<BEGIN" << endl;

        if ( !updateTrainingSet( tempLeafFARate ) ) 
        {
            cout << "Train dataset for temp stage can not be filled."
                "Branch training terminated." << endl;
            break;
        }
        if( tempLeafFARate <= requiredLeafFARate )
        {
            cout << "Required leaf false alarm rate achieved. "
                 "Branch training terminated." << endl;
            break;
        }

        CvCascadeBoost* tempStage = new CvCascadeBoost;
        tempStage->train( (CvFeatureEvaluator*)featureEvaluator,
                           curNumSamples, _numPrecalcVal, _numPrecalcIdx,
                          *((CvCascadeBoostParams*)stageParams) );
        stageClassifiers.push_back( tempStage );

        cout << "END>" << endl;
        
        // save params
        String filename;
        if ( i == 0) 
        {
            filename = dirName + CC_PARAMS_FILENAME;
            FileStorage fs( filename, FileStorage::WRITE);
            if ( !fs.isOpened() )
                return false;
            fs << FileStorage::getDefaultObjectName(filename) << "{";
            writeParams( fs );
            fs << "}";
        }
        // save temp stage
        char buf[10];
        sprintf(buf, "%s%d", "stage", i );
        filename = dirName + buf + ".xml";
        FileStorage fs( filename, FileStorage::WRITE );
        if ( !fs.isOpened() )
            return false;
        fs << FileStorage::getDefaultObjectName(filename) << "{";
        tempStage->write( fs, Mat() );
        fs << "}";
    }
    save( dirName + CC_CASCADE_FILENAME, baseFormatSave );
    return true;
}

int CvCascadeClassifier::predict( int sampleIdx )
{
    CV_DbgAssert( sampleIdx < numPos + numNeg );
    for (Vector< Ptr<CvCascadeBoost> >::iterator it = stageClassifiers.begin();
        it != stageClassifiers.end(); it++ )
    {
        if ( (*it)->predict( sampleIdx ) == 0.f )
            return 0;
    }
    return 1;
}

bool CvCascadeClassifier::updateTrainingSet( double& acceptanceRatio)
{
    int64 posConsumed = 0, negConsumed = 0;
    imgReader.restart();
    int posCount = fillPassedSamles( 0, numPos, true, posConsumed );
    if( !posCount )
        return false;
    cout << "POS count : consumed   " << posCount << " : " << (int)posConsumed << endl;

    int negCount = fillPassedSamles( numPos, numNeg, false, negConsumed );
    if ( !negCount )
        return false;
    curNumSamples = posCount + negCount;
    acceptanceRatio = negConsumed == 0 ? 0 : ( (double)negCount/(double)(int64)negConsumed );
    cout << "NEG count : acceptanceRatio    " << negCount << " : " << acceptanceRatio << endl;
    return true;
}

int CvCascadeClassifier::fillPassedSamles( int first, int count, bool isPositive, int64& consumed )
{
    int getcount = 0;
    Mat img(cascadeParams.winSize, CV_8UC1);
    for( int i = first; i < first + count; i++ )
    {
        for( ; ; )
        {
            bool isGetImg = isPositive ? imgReader.getPos( img ) :
                                           imgReader.getNeg( img );
            if( !isGetImg ) 
                return getcount;
            consumed++;

            featureEvaluator->setImage( img, isPositive ? 1 : 0, i );
            if( predict( i ) == 1.0F )
            {
                getcount++;
                break;
            }
        }
    }
    return getcount;
}

void CvCascadeClassifier::writeParams( FileStorage &fs ) const
{
    cascadeParams.write( fs );
    fs << CC_STAGE_PARAMS << "{"; stageParams->write( fs ); fs << "}";
    fs << CC_FEATURE_PARAMS << "{"; featureParams->write( fs ); fs << "}";
}

void CvCascadeClassifier::writeFeatures( FileStorage &fs, const Mat& featureMap ) const
{
    ((CvFeatureEvaluator*)((Ptr<CvFeatureEvaluator>)featureEvaluator))->writeFeatures( fs, featureMap ); 
}

void CvCascadeClassifier::writeStages( FileStorage &fs, const Mat& featureMap ) const
{
    //char cmnt[30];
    //int i = 0;
    fs << CC_STAGES << "["; 
    for( Vector< Ptr<CvCascadeBoost> >::const_iterator it = stageClassifiers.begin();
        it != stageClassifiers.end(); it++/*, i++*/ )
    {
        /*sprintf( cmnt, "stage %d", i );
        CV_CALL( cvWriteComment( fs, cmnt, 0 ) );*/
        fs << "{";
        ((CvCascadeBoost*)((Ptr<CvCascadeBoost>)*it))->write( fs, featureMap );
        fs << "}";
    }
    fs << "]";
}

bool CvCascadeClassifier::readParams( const FileNode &node )
{
    if ( !node.isMap() || !cascadeParams.read( node ) )
        return false;
    
    stageParams = new CvCascadeBoostParams;
    FileNode rnode = node[CC_STAGE_PARAMS];
    if ( !stageParams->read( rnode ) )
        return false;
    
    featureParams = CvFeatureParams::create(cascadeParams.featureType);
    rnode = node[CC_FEATURE_PARAMS];
    if ( !featureParams->read( rnode ) )
        return false;
    return true;    
}

bool CvCascadeClassifier::readStages( const FileNode &node)
{
    FileNode rnode = node[CC_STAGES];
    if (!rnode.empty() || !rnode.isSeq())
        return false;
    stageClassifiers.reserve(numStages);
    FileNodeIterator it = rnode.begin();
    for( int i = 0; i < min( (int)rnode.size(), numStages ); i++, it++ )
    {
        CvCascadeBoost* tempStage = new CvCascadeBoost;
        if ( !tempStage->read( *it, (CvFeatureEvaluator *)featureEvaluator, *((CvCascadeBoostParams*)stageParams) ) )
        {
            delete tempStage;
            return false;
        }
        stageClassifiers.push_back(tempStage);
    }
    return true;
}

// For old Haar Classifier file saving
#define ICV_HAAR_SIZE_NAME            "size"
#define ICV_HAAR_STAGES_NAME          "stages"
#define ICV_HAAR_TREES_NAME             "trees"
#define ICV_HAAR_FEATURE_NAME             "feature"
#define ICV_HAAR_RECTS_NAME                 "rects"
#define ICV_HAAR_TILTED_NAME                "tilted"
#define ICV_HAAR_THRESHOLD_NAME           "threshold"
#define ICV_HAAR_LEFT_NODE_NAME           "left_node"
#define ICV_HAAR_LEFT_VAL_NAME            "left_val"
#define ICV_HAAR_RIGHT_NODE_NAME          "right_node"
#define ICV_HAAR_RIGHT_VAL_NAME           "right_val"
#define ICV_HAAR_STAGE_THRESHOLD_NAME   "stage_threshold"
#define ICV_HAAR_PARENT_NAME            "parent"
#define ICV_HAAR_NEXT_NAME              "next"

void CvCascadeClassifier::save( const String filename, bool baseFormat )
{
    FileStorage fs( filename, FileStorage::WRITE );

    if ( !fs.isOpened() )
        return;

    fs << FileStorage::getDefaultObjectName(filename) << "{";
    if ( !baseFormat )
    {
        Mat featureMap; 
        getUsedFeaturesIdxMap( featureMap );
        writeParams( fs );
        fs << CC_STAGE_NUM << (int)stageClassifiers.size();
        writeStages( fs, featureMap );
        writeFeatures( fs, featureMap );
    }
    else
    {
        //char buf[256];
        CvSeq* weak;
        if ( cascadeParams.featureType != CvFeatureParams::HAAR )
            CV_Error( CV_StsBadFunc, "old file format is used for Haar-like features only");
        fs << ICV_HAAR_SIZE_NAME << "[:" << cascadeParams.winSize.width << 
            cascadeParams.winSize.height << "]";
        fs << ICV_HAAR_STAGES_NAME << "[";
        for( size_t si = 0; si < stageClassifiers.size(); si++ )
        {
            fs << "{"; //stage
            /*sprintf( buf, "stage %d", si );
            CV_CALL( cvWriteComment( fs, buf, 1 ) );*/
            weak = stageClassifiers[si]->get_weak_predictors();
            fs << ICV_HAAR_TREES_NAME << "[";
            for( int wi = 0; wi < weak->total; wi++ )
            {
                int inner_node_idx = -1, total_inner_node_idx = -1;
                queue<const CvDTreeNode*> inner_nodes_queue;
                CvCascadeBoostTree* tree = *((CvCascadeBoostTree**) cvGetSeqElem( weak, wi ));
                
                fs << "[";
                /*sprintf( buf, "tree %d", wi );
                CV_CALL( cvWriteComment( fs, buf, 1 ) );*/

                const CvDTreeNode* tempNode;
                
                inner_nodes_queue.push( tree->get_root() );
                total_inner_node_idx++;
                
                while (!inner_nodes_queue.empty())
                {
                    tempNode = inner_nodes_queue.front();
                    inner_node_idx++;

                    fs << "{";
                    fs << ICV_HAAR_FEATURE_NAME << "{";
                    ((CvHaarEvaluator*)((CvFeatureEvaluator*)featureEvaluator))->writeFeature( fs, tempNode->split->var_idx );
                    fs << "}";

                    fs << ICV_HAAR_THRESHOLD_NAME << tempNode->split->ord.c;

                    if( tempNode->left->left || tempNode->left->right )
                    {
                        inner_nodes_queue.push( tempNode->left );
                        total_inner_node_idx++;
                        fs << ICV_HAAR_LEFT_NODE_NAME << total_inner_node_idx;
                    }
                    else
                        fs << ICV_HAAR_LEFT_VAL_NAME << tempNode->left->value;

                    if( tempNode->right->left || tempNode->right->right )
                    {
                        inner_nodes_queue.push( tempNode->right );
                        total_inner_node_idx++;
                        fs << ICV_HAAR_RIGHT_NODE_NAME << total_inner_node_idx;
                    }
                    else
                        fs << ICV_HAAR_RIGHT_VAL_NAME << tempNode->right->value;
                    fs << "}"; // ICV_HAAR_FEATURE_NAME
                    inner_nodes_queue.pop();
                }
                fs << "]";
            }
            fs << "]"; //ICV_HAAR_TREES_NAME
            fs << ICV_HAAR_STAGE_THRESHOLD_NAME << stageClassifiers[si]->getThreshold();
            fs << ICV_HAAR_PARENT_NAME << (int)si-1 << ICV_HAAR_NEXT_NAME << -1;
            fs << "}"; //stage
        } /* for each stage */
        fs << "]"; //ICV_HAAR_STAGES_NAME
    }
    fs << "}";
}

bool CvCascadeClassifier::load( const String cascadeDirName )
{
    FileStorage fs( cascadeDirName + CC_PARAMS_FILENAME, FileStorage::READ );
    if ( !fs.isOpened() )
        return false;
    FileNode node = fs.getFirstTopLevelNode();
    if ( !readParams( node ) )
        return false;
    featureEvaluator = CvFeatureEvaluator::create(cascadeParams.featureType);
    featureEvaluator->init( ((CvFeatureParams*)featureParams), numPos + numNeg, cascadeParams.winSize );
    fs.release();

    char buf[10];
    for ( int si = 0; si < numStages; si++ )
    {
        sprintf( buf, "%s%d", "stage", si);
        fs.open( cascadeDirName + buf + ".xml", FileStorage::READ );
        node = fs.getFirstTopLevelNode();
        if ( !fs.isOpened() )
            break;
        CvCascadeBoost *tempStage = new CvCascadeBoost; 

        if ( !tempStage->read( node, (CvFeatureEvaluator*)featureEvaluator, *((CvCascadeBoostParams*)stageParams )) )
        {
            delete tempStage;
            fs.release();
            break;
        }
        stageClassifiers.push_back(tempStage);
    }
    return true;
}

void CvCascadeClassifier::getUsedFeaturesIdxMap( Mat& featureMap )
{
    featureMap.create( 1, featureEvaluator->getNumFeatures(), CV_32SC1 );
    featureMap.setTo(Scalar(-1));
    
    for( Vector< Ptr<CvCascadeBoost> >::const_iterator it = stageClassifiers.begin();
        it != stageClassifiers.end(); it++ )
        ((CvCascadeBoost*)((Ptr<CvCascadeBoost>)(*it)))->markUsedFeaturesInMap( featureMap );
    
    for( int fi = 0, idx = 0; fi < featureEvaluator->getNumFeatures(); fi++ )
        if ( featureMap.at<int>(0, fi) >= 0 )
            featureMap.ptr<int>(0)[fi] = idx++;
}