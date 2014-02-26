#include "L2seedsAnalyzer.h"


L2seedsAnalyzer::L2seedsAnalyzer(const edm::ParameterSet& iConfig)

{
   //now do what ever initialization is needed
    isMC_                   = iConfig.getParameter<bool>("isMC");
    isJPSIonly_             = iConfig.getParameter<bool>("selectJpsiOnly");
    muonProducers_	= iConfig.getParameter<vtag>("muonProducer");
    primaryVertexInputTag_  = iConfig.getParameter<edm::InputTag>("primaryVertexInputTag");
    theSTAMuonLabel_ = iConfig.getUntrackedParameter<std::string>("StandAloneTrackCollectionLabel");
    standAloneAssociatorTag_ = iConfig.getParameter<edm::InputTag>("standAloneAssociator");
    trackingParticlesTag_ =  iConfig.getParameter<edm::InputTag>("trackingParticlesCollection");
    L2seedsTag_ =  iConfig.getParameter<edm::InputTag>("L2seedsCollection");
    L2associatorTag_ = iConfig.getParameter<edm::InputTag>("L2associator");
    L2seedTrackCollectionTag_ = iConfig.getParameter<edm::InputTag>("L2seedTrackCollection");
    theMuonRecHitBuilderName_ = iConfig.getParameter<std::string>("MuonRecHitBuilder");
    associatorLabel_ = iConfig.getParameter< std::string >("associatorLabel");
    outputFile_     = iConfig.getParameter<std::string>("outputFile");
    rootFile_       = TFile::Open(outputFile_.c_str(),"RECREATE");
    isNotFullEventContent = false;

}


L2seedsAnalyzer::~L2seedsAnalyzer()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called for each event  ------------
void
L2seedsAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    using namespace edm;
 //   using namespace std;
    using namespace reco;
    
    
    //muon collection :
    
	edm::Handle < std::vector <reco::Muon> > recoMuons;
    edm::InputTag muonProducer = muonProducers_.at(0);
	iEvent.getByLabel(muonProducer, recoMuons);

    // vertices
    edm::Handle<reco::VertexCollection> vtx_h;
    iEvent.getByLabel(primaryVertexInputTag_, vtx_h);
   // const reco::VertexCollection& vtxs = *(vtx_h.product());
    
    
    // magnetic fied and detector geometry 
    iSetup.get<IdealMagneticFieldRecord>().get(theMGField);
    iSetup.get<GlobalTrackingGeometryRecord>().get(theTrackingGeometry);
    
    
    //stand alone muons tracks
   // Handle<reco::TrackCollection> staTracks;
    edm::Handle<edm::View<reco::Track> > staTracks;
    iEvent.getByLabel(theSTAMuonLabel_, staTracks);
    
    // L2 muons tracks
    edm::Handle<edm::View<reco::Track> > SeedTracks;
    iEvent.getByLabel(L2seedTrackCollectionTag_, SeedTracks);
    
    // gen particles
     edm::Handle <reco::GenParticleCollection> genParticles;
    
    //sim to RECO tracks associator  
    edm::Handle<reco::SimToRecoCollection> simRecoHandle;
    edm::Handle<reco::RecoToSimCollection> recoSimHandle;

    iEvent.getByLabel(standAloneAssociatorTag_,simRecoHandle);
    reco::SimToRecoCollection simRecColl;
    reco::RecoToSimCollection recSimColl;

    edm::LogVerbatim("L2seedsAnalyzer") << "L2 seeds analyzer running";
    
    if (!isNotFullEventContent){
        if (simRecoHandle.isValid()) {
            simRecColl = *(simRecoHandle.product());
        } else {
            edm::LogVerbatim("L2seedsAnalyzer") << "no valid sim RecHit product found ! ";
            isNotFullEventContent = true;
            return;
        }
        //RECO to sim tracks associator
        iEvent.getByLabel(standAloneAssociatorTag_,recoSimHandle);
        if (recoSimHandle.isValid()) {
            recSimColl = *(recoSimHandle.product());
        } else {
            edm::LogVerbatim("L2seedsAnalyzer") << "no valid sim RecHit product found ! " ;
            isNotFullEventContent = true;
            return;
        }
    }
    // tracking particles collection
    edm::Handle<TrackingParticleCollection>  TPCollectionH ;
    TrackingParticleCollection tPC;
    iEvent.getByLabel(trackingParticlesTag_,TPCollectionH);
    if (TPCollectionH.isValid()) tPC   = *(TPCollectionH.product());
    else edm::LogVerbatim("L2seedsAnalyzer") << "not found tracking particles collection";

    
   // now read the L2 seeds collection :
    edm::Handle<TrajectorySeedCollection> L2seedsCollection;
    iEvent.getByLabel(L2seedsTag_,L2seedsCollection);
    const std::vector<TrajectorySeed>* L2seeds = 0;
    if (L2seedsCollection.isValid()) L2seeds = L2seedsCollection.product();
    else edm::LogVerbatim("L2seedsAnalyzer") << "L2 seeds collection not found !! ";
    
    
    edm::ESHandle<TransientTrackingRecHitBuilder> theMuonRecHitBuilder;
    iSetup.get<TransientRecHitRecord>().get(theMuonRecHitBuilderName_,theMuonRecHitBuilder);
    
    
    //sim to L2 seeds associator
    edm::Handle<reco::SimToRecoCollection> L2simRecoHandle;
    iEvent.getByLabel(L2associatorTag_,L2simRecoHandle);
    reco::SimToRecoCollection L2simRecColl;
    //RECO to sim L2 seeds associator
    edm::Handle<reco::RecoToSimCollection> L2recoSimHandle;
    iEvent.getByLabel(L2associatorTag_,L2recoSimHandle);
    reco::RecoToSimCollection L2recSimColl;
    
    if (!isNotFullEventContent){
        if (L2simRecoHandle.isValid()) {
            L2simRecColl = *(L2simRecoHandle.product());
        } else {
            edm::LogVerbatim("L2seedsAnalyzer") << "no valid sim RecHit product found ! ";
            isNotFullEventContent = true;
            return;
        }
        if (L2recoSimHandle.isValid()) {
            L2recSimColl = *(L2recoSimHandle.product());
        } else {
            edm::LogVerbatim("L2seedsAnalyzer") << "no valid sim RecHit product found ! ";
            isNotFullEventContent = true;
            return;
        }
    }
    
    edm::ESHandle<TrackAssociatorBase> associatorBase;
    iSetup.get<TrackAssociatorRecord>().get(associatorLabel_, associatorBase);
    const MuonAssociatorByHits * assoByHits = dynamic_cast<const MuonAssociatorByHits *>(associatorBase.product());
   // MuonAssociatorByHits::MuonToSimCollection UpdSTA_recSimColl;
    //MuonAssociatorByHits::SimToMuonCollection UpdSTA_simRecColl;
    
    edm::RefVector<TrackingParticleCollection> allTPs;
    for (size_t i = 0, n = TPCollectionH->size(); i < n; ++i) {
        allTPs.push_back(TrackingParticleRef(TPCollectionH,i));
    }
   // assoByHits->associateMuons(UpdSTA_recSimColl, UpdSTA_simRecColl, staTracks, MuonAssociatorByHits::OuterTk, allTPs, &iEvent, &iSetup);
    
    if (isNotFullEventContent){
        recSimColl = assoByHits->associateRecoToSim(staTracks, TPCollectionH, &iEvent, &iSetup);
        simRecColl = assoByHits->associateSimToReco(staTracks, TPCollectionH, &iEvent, &iSetup);
        
        L2recSimColl = assoByHits->associateRecoToSim(SeedTracks, TPCollectionH, &iEvent, &iSetup);
        L2simRecColl = assoByHits->associateSimToReco(SeedTracks, TPCollectionH, &iEvent, &iSetup);
        
    }
    
    
    
    beginEvent();
    
    reco::Vertex dummy;
    const reco::Vertex *pv = &dummy;
    if (vtx_h->size() != 0) {
        pv = &*vtx_h->begin();
    } else { // create a dummy PV
        Vertex::Error e;
        e(0, 0) = 0.0015 * 0.0015;
        e(1, 1) = 0.0015 * 0.0015;
        e(2, 2) = 15. * 15.;
        Vertex::Point p(0, 0, 0);
        dummy = Vertex(p, e, 0, 0, 0);
    }
    
    T_Event_EventNumber = iEvent.id().event();
    
   /* int checkEvents[1] = {5709};
    bool goodEvent = false;
    for (int i = 0 ; i < 1 ; i++){
        if (T_Event_EventNumber == checkEvents[i]) goodEvent = true;
    }
    if (!(goodEvent)) return;*/
    //cout << "event=" << T_Event_EventNumber << endl;

    int nbMuons = recoMuons->size();
    //cout << "there are " << nbMuons << " muons in the event" << endl;
   
    //loop on the reco muons in the event
    for (int k = 0 ; k < nbMuons ; k++){
        
        const reco::Muon* muon = &((*recoMuons)[k]);
        
        T_Muon_Eta->push_back(muon->eta());
        T_Muon_Phi->push_back(muon->phi());
        T_Muon_IsGlobalMuon->push_back(muon->isGlobalMuon());
        T_Muon_IsPFMuon->push_back(muon->isPFMuon());
        T_Muon_IsTrackerMuon->push_back(muon->isTrackerMuon());
        T_Muon_IsCaloMuon->push_back(muon->isCaloMuon());
        T_Muon_IsStandAloneMuon->push_back(muon->isStandAloneMuon());
        T_Muon_IsMuon->push_back(muon->isMuon());
        T_Muon_Energy->push_back(muon->energy());
        T_Muon_Et->push_back(muon->et());
        T_Muon_Pt->push_back(muon->pt());
        T_Muon_Px->push_back(muon->px());
        T_Muon_Py->push_back(muon->py());
        T_Muon_Pz->push_back(muon->pz());
        T_Muon_Mass->push_back(muon->mass());
        T_Muon_charge->push_back(muon->charge());
    
      //  cout << "info muon" << endl;
        T_Muon_numberOfChambers->push_back(muon->numberOfChambers());
        T_Muon_numberOfChambersRPC->push_back(muon->numberOfChambersNoRPC());
        T_Muon_numberOfMatches->push_back(muon->numberOfMatches());
        T_Muon_numberOfMatchedStations->push_back(muon->numberOfMatchedStations());
        bool isMatchTheStation = muon::isGoodMuon(*muon, muon::TMOneStationTight);
        bool isGlobalMuonPT = muon::isGoodMuon(*muon, muon::GlobalMuonPromptTight);
        bool isGlobalMuonArbitrated = muon::isGoodMuon(*muon, muon::TrackerMuonArbitrated);
        T_Muon_TMLastStationTight->push_back(isMatchTheStation);
        T_Muon_IsGlobalMuon_PromptTight->push_back(isGlobalMuonPT);
        T_Muon_IsTrackerMuonArbitrated->push_back(isGlobalMuonArbitrated);
        
     //   cout << "last info muon" << endl;
        if (muon->globalTrack().isNull()) T_Muon_globalTrackChi2->push_back(-1); else T_Muon_globalTrackChi2->push_back(muon->globalTrack()->normalizedChi2());
        if (muon->globalTrack().isNull()) T_Muon_validMuonHits->push_back(-1); else T_Muon_validMuonHits->push_back(muon->globalTrack()->hitPattern().numberOfValidMuonHits());
        T_Muon_trkKink->push_back(muon->combinedQuality().trkKink);
        if (muon->muonBestTrack().isNull()) {
            T_Muon_trkNbOfTrackerLayers->push_back(-1);
            T_Muon_trkError->push_back(-1);
            T_Muon_dB->push_back(-1);
            T_Muon_dzPV->push_back(-1);
            T_Muon_trkValidPixelHits->push_back(-1);
            T_Muon_trkNbOfValidTrackeHits->push_back(-1);
        }
        else {
            T_Muon_trkNbOfTrackerLayers->push_back(muon->muonBestTrack()->hitPattern().trackerLayersWithMeasurement());
            T_Muon_trkError->push_back(muon->muonBestTrack()->ptError());
            T_Muon_trkValidPixelHits->push_back(muon->muonBestTrack()->hitPattern().numberOfValidPixelHits());
            T_Muon_dB->push_back(fabs(muon->muonBestTrack()->dxy(pv->position())));
            T_Muon_dzPV->push_back(fabs(muon->muonBestTrack()->dz(pv->position())));
            T_Muon_trkNbOfValidTrackeHits->push_back(muon->muonBestTrack()->hitPattern().numberOfValidTrackerHits());
        }
        
    
    
    }
    
    
   if(isMC_){
        edm::Handle<GenEventInfoProduct> genEvent;
        iEvent.getByLabel("generator", genEvent);
        iEvent.getByLabel("genParticles", genParticles );
       
       Handle<std::vector< PileupSummaryInfo > > puInfo;
       try {
           iEvent.getByLabel("addPileupInfo",puInfo);
           std::vector<PileupSummaryInfo>::const_iterator PVI;
           //The in-time crossing is getBunchCrossing = 0; negative ones are early, positive ones are late.
           for(PVI = puInfo->begin(); PVI != puInfo->end(); ++PVI) {
               
               //    std::cout << " Pileup Information: bunchXing, nvtx: " << PVI->getBunchCrossing() << " " << PVI->getPU_NumInteractions() << std::endl;
               if(PVI->getBunchCrossing()==0){
                   T_Event_nPU =PVI->getPU_NumInteractions();
                   T_Event_nTruePU=PVI->getTrueNumInteractions();
                   
               }
               
               else if(PVI->getBunchCrossing()==-1){
                   T_Event_nPUm=PVI->getPU_NumInteractions();
               }
               else if(PVI->getBunchCrossing()==1){
                   T_Event_nPUp=PVI->getPU_NumInteractions();
               }
               //truePu += PVI->getTrueNumInteractions();
           }
       } catch (...) {}
       
        int nbOfGen = genParticles->size();
        for (int j = 0 ; j < nbOfGen ; j++){
            const reco::GenParticle & theCand = (*genParticles)[j];
            int theMotherID = 0;
            const reco::Candidate * theLocalCandidate = &theCand;
            bool hasMother = (theLocalCandidate->numberOfMothers()>0);
            const reco::Candidate * theMother;
            while (hasMother) {//check if the muon has a J/Psi in its mother particles
                theMother = theLocalCandidate->mother();
                theLocalCandidate = theMother;
                hasMother = (theLocalCandidate->numberOfMothers()>0);
                theMotherID = theMother->pdgId();
                if (theMother->pdgId()==443) break;
            }
            if (isJPSIonly_ && theMotherID!=443) continue;
            edm::LogVerbatim("L2seedsAnalyzer") << "gen muon:  eta=" << theCand.eta() << ", " << theCand.phi() << ", pt=" << theCand.pt();
            T_Gen_Muon_Px->push_back(theCand.px());
            T_Gen_Muon_Py->push_back(theCand.py());
            T_Gen_Muon_Pz->push_back(theCand.pz());
            T_Gen_Muon_Pt->push_back(theCand.pt());
            T_Gen_Muon_Phi->push_back(theCand.phi());
            T_Gen_Muon_Eta->push_back(theCand.eta());
            T_Gen_Muon_Energy->push_back(theCand.energy());
            T_Gen_Muon_PDGid->push_back(theCand.pdgId());
            T_Gen_Muon_status->push_back(theCand.status());
        
	    //cout << "part=" << theCand.pdgId() << " motherID=" << theMotherID << endl;
            T_Gen_Muon_MotherID->push_back(theMotherID);
            for (TrackingParticleCollection::size_type i=0; i<tPC.size(); i++) {
                TrackingParticleRef trpart(TPCollectionH, i);
                float deltaRtp = sqrt(pow(trpart->eta()-theCand.eta(),2)+ pow(acos(cos(trpart->phi()-theCand.phi())),2)) ;
                float detlaPttp = fabs(trpart->pt()-theCand.pt())/theCand.pt();
                if ((deltaRtp < 0.01)&&(detlaPttp<0.05)){
                    edm::LogVerbatim("L2seedsAnalyzer") << "after Matching= Pt=" <<trpart->pt() << " eta=" <<trpart->eta() << " phi=" << trpart->phi();
                    T_Gen_Muon_tpPt->push_back(trpart->pt());
                    T_Gen_Muon_tpEta->push_back(trpart->eta());
                    T_Gen_Muon_tpPhi->push_back(trpart->phi());
                    // now look for a STD muon
                    //edm::RefToBase<reco::Track> *theSTAMuon = 0;
                    
                    bool isTrackFound = false;
                //    if (!isNotFullEventContent){
                        float matchQuality, matchPurity;
                        edm::RefToBase<reco::Track> theSTAMuon = findAstaMuon(trpart, simRecColl, recSimColl, &isTrackFound, &matchQuality, &matchPurity, iSetup);
                        edm::LogVerbatim("L2seedsAnalyzer") << "found=" << isTrackFound << " quality=" << matchQuality << " purity=" << matchPurity;
                        if (isTrackFound) edm::LogVerbatim("L2seedsAnalyzer") << "STA muon PT=" << (theSTAMuon)->pt();
                        if (isTrackFound) edm::LogVerbatim("L2seedsAnalyzer") << "STA muon eta=" << (theSTAMuon)->eta() << " phi=" << (theSTAMuon)->phi();
                        if (isTrackFound) {
                            T_Gen_Muon_FoundSTA->push_back(1);
                            T_Gen_Muon_StaPt->push_back(theSTAMuon->pt());
                            T_Gen_Muon_StaEta->push_back(theSTAMuon->eta());
                            T_Gen_Muon_StaPhi->push_back(theSTAMuon->phi());
                            T_Gen_Muon_StaPurity->push_back(matchPurity);
                            T_Gen_Muon_StaQuality->push_back(matchQuality);
                            TrajectorySeed theSeed = (*theSTAMuon->seedRef());
                            const TrackingRecHit *seghit = &(*(theSeed.recHits().first));
                            TransientTrackingRecHit::ConstRecHitPointer ttrh(theMuonRecHitBuilder->build(seghit));
                            T_Gen_Muon_StaSeedEta->push_back(ttrh->globalPosition().eta());
                            T_Gen_Muon_StaSeedPhi->push_back(ttrh->globalPosition().phi());
                            edm::LogVerbatim("L2seedsAnalyzer") << "found a STA with seed = Eta=" << ttrh->globalPosition().eta() << " phi=" << ttrh->globalPosition().phi();
                            edm::LogVerbatim("L2seedsAnalyzer") << "Pt=" << theSTAMuon->pt();
                        }
                        else {
                            T_Gen_Muon_FoundSTA->push_back(0);
                            T_Gen_Muon_StaPt->push_back(-1);
                            T_Gen_Muon_StaEta->push_back(-1);
                            T_Gen_Muon_StaPhi->push_back(-1);
                            T_Gen_Muon_StaPurity->push_back(-1);
                            T_Gen_Muon_StaQuality->push_back(-1);
                            T_Gen_Muon_StaSeedEta->push_back(-1);
                            T_Gen_Muon_StaSeedPhi->push_back(-1);
                        }
                     //if (!isNotFullEventContent){

                        // now look if found a match with a L2 seed
                        bool isL2seedFound = false;
                        float matchQualityL2, matchPurityL2;
                        edm::RefToBase<reco::Track> theL2seed = findAstaMuon(trpart, L2simRecColl, L2recSimColl, &isL2seedFound, &matchQualityL2, &matchPurityL2, iSetup);
                        if (isL2seedFound){
                            T_Gen_Muon_FoundL2->push_back(1);
                            TrajectorySeed theSeed = (*theL2seed->seedRef());
                            const TrackingRecHit *seghit = &(*(theSeed.recHits().first));
                            TransientTrackingRecHit::ConstRecHitPointer ttrh(theMuonRecHitBuilder->build(seghit));
                            //cout << "phi=" << ttrh->globalPosition().phi() << endl;
                            // cout << "eta=" << ttrh->globalPosition().eta() << endl;
                            T_Gen_Muon_L2Eta->push_back(ttrh->globalPosition().eta());
                            T_Gen_Muon_L2Phi->push_back(ttrh->globalPosition().phi());
                            edm::LogVerbatim("L2seedsAnalyzer") << "found a L2  seed = Eta=" << ttrh->globalPosition().eta() << " phi=" << ttrh->globalPosition().phi();
                            edm::LogVerbatim("L2seedsAnalyzer") << "Pt=" << theSeed.startingState().parameters().momentum().perp();

                            T_Gen_Muon_L2Purity->push_back(matchPurityL2);
                            T_Gen_Muon_L2Quality->push_back(matchQualityL2);
                        
                        }
                        else {
                            T_Gen_Muon_FoundL2->push_back(0);
                            T_Gen_Muon_L2Eta->push_back(-1);
                            T_Gen_Muon_L2Phi->push_back(-1);
                            T_Gen_Muon_L2Purity->push_back(-1);
                            T_Gen_Muon_L2Quality->push_back(-1);
                        }

                  //  }
                    /// now perform a crude matching with a dR cone
                    int foundACrudeMatching = false;
                    for (unsigned int i = 0; i < L2seeds->size() ; i++){
                        const TrackingRecHit *seghit = &(*((L2seeds->at(i)).recHits().first));
                        TransientTrackingRecHit::ConstRecHitPointer ttrh(theMuonRecHitBuilder->build(seghit));
                        float dRseed = deltaR(ttrh->globalPosition().phi(), theCand.phi(), ttrh->globalPosition().eta(), theCand.eta());
                        if (dRseed<0.5) foundACrudeMatching = true;
                        //cout << "dRseed" << dRseed << endl;
                    }
                    if (foundACrudeMatching) T_Gen_Muon_L2crudeMaching->push_back(1);
                    else T_Gen_Muon_L2crudeMaching->push_back(0);
                }
                else //not found a tracking particle, fill all variables to -1 in order to have same nb of entries in TP and Gen particles trees...
                 {
                     T_Gen_Muon_tpPt->push_back(-1);
                     T_Gen_Muon_tpEta->push_back(-1);
                     T_Gen_Muon_tpPhi->push_back(-1);
                     T_Gen_Muon_FoundSTA->push_back(0);
                     T_Gen_Muon_StaPt->push_back(-1);
                     T_Gen_Muon_StaEta->push_back(-1);
                     T_Gen_Muon_StaPhi->push_back(-1);
                     T_Gen_Muon_StaPurity->push_back(-1);
                     T_Gen_Muon_StaQuality->push_back(-1);
                     T_Gen_Muon_StaSeedEta->push_back(-1);
                     T_Gen_Muon_StaSeedPhi->push_back(-1);
                     T_Gen_Muon_FoundL2->push_back(0);
                     T_Gen_Muon_L2Eta->push_back(-1);
                     T_Gen_Muon_L2Phi->push_back(-1);
                     T_Gen_Muon_L2Purity->push_back(-1);
                     T_Gen_Muon_L2Quality->push_back(-1);
                     T_Gen_Muon_L2crudeMaching->push_back(0);
                }
            }
        }
    }
    // now try a loop on the seeds :
    int countRH = 0;
    for(TrajectorySeedCollection::const_iterator seed = L2seeds->begin(); seed != L2seeds->end(); ++seed){
        T_Seed_Muon_nHits->push_back(seed->nHits() );
        T_Seed_Muon_refFirstHit->push_back(countRH);
        //cout << "coucou dans la seed, nHits=" << seed->nHits() << "first Hits=" << countRH << endl;
        TrajectoryStateOnSurface theTrajectory = seedTransientState(*seed);
        T_Seed_Muon_Eta->push_back(theTrajectory.globalMomentum().eta());
        T_Seed_Muon_Phi->push_back(theTrajectory.globalMomentum().phi());
        T_Seed_Muon_EtaErr->push_back(theTrajectory.curvilinearError().matrix()(1,1));
        T_Seed_Muon_PhiErr->push_back(theTrajectory.curvilinearError().matrix()(2,2));
        T_Seed_Muon_Pt->push_back(theTrajectory.globalMomentum().perp());
        T_Seed_Muon_Px->push_back(theTrajectory.globalMomentum().x());
        T_Seed_Muon_Py->push_back(theTrajectory.globalMomentum().y());
        T_Seed_Muon_Pz->push_back(theTrajectory.globalMomentum().z());
        
        AlgebraicSymMatrix66 errors = theTrajectory.cartesianError().matrix();
        double partialPterror = errors(3,3)*pow(theTrajectory.globalMomentum().x(),2) + errors(4,4)*pow(theTrajectory.globalMomentum().y(),2);
        T_Seed_Muon_PtErr->push_back(sqrt(partialPterror));
        T_Seed_Muon_PxErr->push_back(sqrt(errors(3,3)));
        T_Seed_Muon_PyErr->push_back(sqrt(errors(4,4)));
        T_Seed_Muon_PzErr->push_back(sqrt(errors(5,5)));
        
        //now loop on the recHits
        for(TrajectorySeed::recHitContainer::const_iterator itRecHits=seed->recHits().first; itRecHits!=seed->recHits().second; ++itRecHits, ++countRH) {
            const TrackingRecHit *seghit = &(*itRecHits);
            if((*seghit).isValid()) {
                TransientTrackingRecHit::ConstRecHitPointer ttrh(theMuonRecHitBuilder->build(seghit));
                T_Hits_Muon_L2Eta->push_back(ttrh->globalPosition().eta());
                T_Hits_Muon_L2Phi->push_back(ttrh->globalPosition().phi());
            }
            else {
                T_Hits_Muon_L2Eta->push_back(-1);
                T_Hits_Muon_L2Phi->push_back(-1);
            }
            T_Hits_Muon_localL2x->push_back((*itRecHits).localPosition().x());
            T_Hits_Muon_localL2y->push_back((*itRecHits).localPosition().y());
            T_Hits_Muon_localL2z->push_back((*itRecHits).localPosition().z());
            
            DetId geoid = (*itRecHits).geographicalId();
            unsigned int detid = geoid.rawId();
            //DetId::Detector det = geoid.det();
            int subdet = geoid.subdetId();
               if (subdet == MuonSubdetId::DT)
                {
                    T_Hits_Muon_isDT->push_back(1);
                    DTWireId dtdetid = DTWireId(detid);
                    T_Hits_Muon_DTwire->push_back(dtdetid.wire());
                    T_Hits_Muon_DTlayer->push_back(dtdetid.layerId().layer());
                    T_Hits_Muon_DTsuperlayer->push_back(dtdetid.layerId().superlayerId().superlayer());
                    T_Hits_Muon_DTWheel->push_back(dtdetid.layerId().superlayerId().chamberId().wheel());
                    T_Hits_Muon_DTStation->push_back(dtdetid.layerId().superlayerId().chamberId().station());
                    T_Hits_Muon_DTSector->push_back(dtdetid.layerId().superlayerId().chamberId().sector());
                    
                }
                else {
                    T_Hits_Muon_isDT->push_back(0);
                    T_Hits_Muon_DTwire->push_back(-99);
                    T_Hits_Muon_DTlayer->push_back(-99);
                    T_Hits_Muon_DTsuperlayer->push_back(-99);
                    T_Hits_Muon_DTWheel->push_back(-99);
                    T_Hits_Muon_DTStation->push_back(-99);
                    T_Hits_Muon_DTSector->push_back(-99);
                    
                }
                if (subdet == MuonSubdetId::CSC){
                    T_Hits_Muon_isCSC->push_back(1);
                    CSCDetId cscdetid = CSCDetId(detid);
                    T_Hits_Muon_isCSC->push_back(cscdetid.layer());
                    T_Hits_Muon_CSClayer->push_back(cscdetid.chamber());
                    T_Hits_Muon_CSCchamber->push_back(cscdetid.ring());
                    T_Hits_Muon_CSCring->push_back(cscdetid.station());
                    T_Hits_Muon_CSCiChamberType->push_back(cscdetid.iChamberType());

                    
                }
                else {
                    T_Hits_Muon_isCSC->push_back(0);
                    T_Hits_Muon_CSClayer->push_back(-99);
                    T_Hits_Muon_CSCchamber->push_back(-99);
                    T_Hits_Muon_CSCring->push_back(-99);
                    T_Hits_Muon_CSCiChamberType->push_back(-99);
                }
        }
    

        
     }


    mytree_->Fill();
    
    endEvent();
}


// ------------ method called once each job just before starting event loop  ------------
void 
L2seedsAnalyzer::beginJob()
{
    mytree_ = new TTree("eventsTree","");
    
    mytree_->Branch("T_Event_RunNumber", &T_Event_RunNumber, "T_Event_RunNumber/I");
    mytree_->Branch("T_Event_EventNumber", &T_Event_EventNumber, "T_Event_EventNumber/I");
    mytree_->Branch("T_Event_LuminosityBlock", &T_Event_LuminosityBlock, "T_Event_LuminosityBlock/I");
    mytree_->Branch("T_Event_nPU", &T_Event_nPU, "T_Event_nPU/I");
    mytree_->Branch("T_Event_nTruePU", &T_Event_nTruePU, "T_Event_nTruePU/I");
    mytree_->Branch("T_Event_nPUm", &T_Event_nPUm, "T_Event_nPUm/I");
    mytree_->Branch("T_Event_nPUp", &T_Event_nPUp, "T_Event_nPUp/I");

    mytree_->Branch("T_Muon_Eta", "std::vector<float>", &T_Muon_Eta);
    mytree_->Branch("T_Muon_Phi", "std::vector<float>", &T_Muon_Phi);
    mytree_->Branch("T_Muon_Energy", "std::vector<float>", &T_Muon_Energy);
    mytree_->Branch("T_Muon_Et", "std::vector<float>", &T_Muon_Et);
    mytree_->Branch("T_Muon_Pt", "std::vector<float>", &T_Muon_Pt);
    mytree_->Branch("T_Muon_Px", "std::vector<float>", &T_Muon_Px);
    mytree_->Branch("T_Muon_Py", "std::vector<float>", &T_Muon_Py);
    mytree_->Branch("T_Muon_Pz", "std::vector<float>", &T_Muon_Pz);
    mytree_->Branch("T_Muon_Mass", "std::vector<float>", &T_Muon_Mass);
    
    mytree_->Branch("T_Muon_IsGlobalMuon", "std::vector<bool>", &T_Muon_IsGlobalMuon);
    mytree_->Branch("T_Muon_IsTrackerMuon", "std::vector<bool>", &T_Muon_IsTrackerMuon);
    mytree_->Branch("T_Muon_IsPFMuon", "std::vector<bool>", &T_Muon_IsPFMuon);
    mytree_->Branch("T_Muon_IsCaloMuon", "std::vector<bool>", &T_Muon_IsCaloMuon);
    mytree_->Branch("T_Muon_IsStandAloneMuon", "std::vector<bool>", &T_Muon_IsStandAloneMuon);
    mytree_->Branch("T_Muon_IsMuon", "std::vector<bool>", &T_Muon_IsMuon);
    mytree_->Branch("T_Muon_IsGlobalMuon_PromptTight", "std::vector<bool>", &T_Muon_IsGlobalMuon_PromptTight);
    mytree_->Branch("T_Muon_IsTrackerMuonArbitrated", "std::vector<bool>", &T_Muon_IsTrackerMuonArbitrated);
    mytree_->Branch("T_Muon_numberOfChambers", "std::vector<int>", &T_Muon_numberOfChambers);
    mytree_->Branch("T_Muon_numberOfChambersRPC", "std::vector<int>", &T_Muon_numberOfChambersRPC);
    mytree_->Branch("T_Muon_numberOfMatches", "std::vector<int>", &T_Muon_numberOfMatches);
    mytree_->Branch("T_Muon_numberOfMatchedStations", "std::vector<int>", &T_Muon_numberOfMatchedStations);
    mytree_->Branch("T_Muon_charge", "std::vector<int>", &T_Muon_charge);

    mytree_->Branch("T_Muon_TMLastStationTight", "std::vector<bool>", &T_Muon_TMLastStationTight);
    mytree_->Branch("T_Muon_globalTrackChi2", "std::vector<float>", &T_Muon_globalTrackChi2);
    mytree_->Branch("T_Muon_validMuonHits", "std::vector<int>", &T_Muon_validMuonHits);
    mytree_->Branch("T_Muon_trkKink", "std::vector<float>", &T_Muon_trkKink);
    mytree_->Branch("T_Muon_trkNbOfTrackerLayers", "std::vector<int>", &T_Muon_trkNbOfTrackerLayers);
    mytree_->Branch("T_Muon_trkNbOfValidTrackeHits", "std::vector<int>", &T_Muon_trkNbOfValidTrackeHits);
    mytree_->Branch("T_Muon_trkValidPixelHits", "std::vector<int>", &T_Muon_trkValidPixelHits);
    mytree_->Branch("T_Muon_trkError", "std::vector<float>", &T_Muon_trkError);
    mytree_->Branch("T_Muon_dB", "std::vector<float>", &T_Muon_dB);
    mytree_->Branch("T_Muon_dzPV", "std::vector<float>", &T_Muon_dzPV);
    
    
    mytree_->Branch("T_Seed_Muon_nHits", "std::vector<int>", &T_Seed_Muon_nHits);
    mytree_->Branch("T_Seed_Muon_refFirstHit", "std::vector<int>", &T_Seed_Muon_refFirstHit);
    mytree_->Branch("T_Seed_Muon_Eta", "std::vector<float>", &T_Seed_Muon_Eta);
    mytree_->Branch("T_Seed_Muon_Phi", "std::vector<float>", &T_Seed_Muon_Phi);
    mytree_->Branch("T_Seed_Muon_Pt", "std::vector<float>", &T_Seed_Muon_Pt);
    mytree_->Branch("T_Seed_Muon_Pz", "std::vector<float>", &T_Seed_Muon_Pz);
    mytree_->Branch("T_Seed_Muon_Px", "std::vector<float>", &T_Seed_Muon_Px);
    mytree_->Branch("T_Seed_Muon_Py", "std::vector<float>", &T_Seed_Muon_Py);
    mytree_->Branch("T_Seed_Muon_PxErr", "std::vector<float>", &T_Seed_Muon_PxErr);
    mytree_->Branch("T_Seed_Muon_PyErr", "std::vector<float>", &T_Seed_Muon_PyErr);
    mytree_->Branch("T_Seed_Muon_PzErr", "std::vector<float>", &T_Seed_Muon_PzErr);
    mytree_->Branch("T_Seed_Muon_PtErr", "std::vector<float>", &T_Seed_Muon_PtErr);
    mytree_->Branch("T_Seed_Muon_EtaErr", "std::vector<float>", &T_Seed_Muon_EtaErr);
    mytree_->Branch("T_Seed_Muon_PhiErr", "std::vector<float>", &T_Seed_Muon_PhiErr);

    mytree_->Branch("T_Hits_Muon_L2Eta", "std::vector<float>", &T_Hits_Muon_L2Eta);
    mytree_->Branch("T_Hits_Muon_L2Phi", "std::vector<float>", &T_Hits_Muon_L2Phi);
    mytree_->Branch("T_Hits_Muon_localL2x", "std::vector<float>", &T_Hits_Muon_localL2x);
    mytree_->Branch("T_Hits_Muon_localL2y", "std::vector<float>", &T_Hits_Muon_localL2y);
    mytree_->Branch("T_Hits_Muon_localL2z", "std::vector<float>", &T_Hits_Muon_localL2z);
    mytree_->Branch("T_Hits_Muon_isDT", "std::vector<int>", &T_Hits_Muon_isDT);
    mytree_->Branch("T_Hits_Muon_DTwire", "std::vector<int>", &T_Hits_Muon_DTwire);
    mytree_->Branch("T_Hits_Muon_DTlayer", "std::vector<int>", &T_Hits_Muon_DTlayer);
    mytree_->Branch("T_Hits_Muon_DTsuperlayer", "std::vector<int>", &T_Hits_Muon_DTsuperlayer);
    mytree_->Branch("T_Hits_Muon_DTWheel", "std::vector<int>", &T_Hits_Muon_DTWheel);
    mytree_->Branch("T_Hits_Muon_DTStation", "std::vector<int>", &T_Hits_Muon_DTStation);
    mytree_->Branch("T_Hits_Muon_DTSector", "std::vector<int>", &T_Hits_Muon_DTSector);
    mytree_->Branch("T_Hits_Muon_isCSC", "std::vector<int>", &T_Hits_Muon_isCSC);
    mytree_->Branch("T_Hits_Muon_CSClayer", "std::vector<int>", &T_Hits_Muon_CSClayer);
    mytree_->Branch("T_Hits_Muon_CSCchamber", "std::vector<int>", &T_Hits_Muon_CSCchamber);
    mytree_->Branch("T_Hits_Muon_CSCring", "std::vector<int>", &T_Hits_Muon_CSCring);
    mytree_->Branch("T_Hits_Muon_CSCiChamberType", "std::vector<int>", &T_Hits_Muon_CSCiChamberType);


    if (isMC_){
        mytree_->Branch("T_Gen_Muon_Px", "std::vector<float>", &T_Gen_Muon_Px);
        mytree_->Branch("T_Gen_Muon_Py", "std::vector<float>", &T_Gen_Muon_Py);
        mytree_->Branch("T_Gen_Muon_Pz", "std::vector<float>", &T_Gen_Muon_Pz);
        mytree_->Branch("T_Gen_Muon_Energy", "std::vector<float>", &T_Gen_Muon_Energy);
        mytree_->Branch("T_Gen_Muon_Pt", "std::vector<float>", &T_Gen_Muon_Pt);
        mytree_->Branch("T_Gen_Muon_Eta", "std::vector<float>", &T_Gen_Muon_Eta);
        mytree_->Branch("T_Gen_Muon_Phi", "std::vector<float>", &T_Gen_Muon_Phi);
        mytree_->Branch("T_Gen_Muon_PDGid", "std::vector<int>", &T_Gen_Muon_PDGid);
        mytree_->Branch("T_Gen_Muon_status", "std::vector<int>", &T_Gen_Muon_status);
        mytree_->Branch("T_Gen_Muon_MotherID", "std::vector<int>", &T_Gen_Muon_MotherID);
        mytree_->Branch("T_Gen_Muon_tpPt", "std::vector<float>", &T_Gen_Muon_tpPt);
        mytree_->Branch("T_Gen_Muon_tpEta", "std::vector<float>", &T_Gen_Muon_tpEta);
        mytree_->Branch("T_Gen_Muon_tpPhi", "std::vector<float>", &T_Gen_Muon_tpPhi);
        mytree_->Branch("T_Gen_Muon_FoundSTA", "std::vector<int>", &T_Gen_Muon_FoundSTA);
        mytree_->Branch("T_Gen_Muon_StaPt", "std::vector<float>", &T_Gen_Muon_StaPt);
        mytree_->Branch("T_Gen_Muon_StaEta", "std::vector<float>", &T_Gen_Muon_StaEta);
        mytree_->Branch("T_Gen_Muon_StaPhi", "std::vector<float>", &T_Gen_Muon_StaPhi);
        mytree_->Branch("T_Gen_Muon_StaPurity", "std::vector<float>", &T_Gen_Muon_StaPurity);
        mytree_->Branch("T_Gen_Muon_StaQuality", "std::vector<float>", &T_Gen_Muon_StaQuality);
        mytree_->Branch("T_Gen_Muon_StaSeedEta", "std::vector<float>", &T_Gen_Muon_StaSeedEta);
        mytree_->Branch("T_Gen_Muon_StaSeedPhi", "std::vector<float>", &T_Gen_Muon_StaSeedPhi);
        mytree_->Branch("T_Gen_Muon_FoundL2", "std::vector<int>", &T_Gen_Muon_FoundL2);
        mytree_->Branch("T_Gen_Muon_L2Eta", "std::vector<float>", &T_Gen_Muon_L2Eta);
        mytree_->Branch("T_Gen_Muon_L2Phi", "std::vector<float>", &T_Gen_Muon_L2Phi);
        mytree_->Branch("T_Gen_Muon_L2Purity", "std::vector<float>", &T_Gen_Muon_L2Purity);
        mytree_->Branch("T_Gen_Muon_L2Quality", "std::vector<float>", &T_Gen_Muon_L2Quality);
        mytree_->Branch("T_Gen_Muon_L2crudeMaching", "std::vector<int>", &T_Gen_Muon_L2crudeMaching);
    }


}


edm::RefToBase<reco::Track>
L2seedsAnalyzer::findAstaMuon(TrackingParticleRef trpart, reco::SimToRecoCollection simRecColl, reco::RecoToSimCollection recSimColl, bool *trackFound, float *theMatchQuality, float *theMatchPurity, const edm::EventSetup& iSetup){
    edm::ESHandle<TransientTrackingRecHitBuilder> theLocalMuonRecHitBuilder;
    iSetup.get<TransientRecHitRecord>().get(theMuonRecHitBuilderName_,theLocalMuonRecHitBuilder);
    
    //1) find the STA muons if there is.
    //cout << "matching in progress" << endl;
    bool foundAmatch=false;
    edm::RefToBase<reco::Track> theBestQualitySTA; //will store the STA with the best quality !
    float bestQuality=0; //initial value
   // cout << bestQuality << endl;
    std::vector<std::pair<edm::RefToBase<reco::Track>, double> > simRecAsso;
    if(simRecColl.find(trpart) != simRecColl.end()) {
        simRecAsso = (std::vector<std::pair<edm::RefToBase<reco::Track>, double> >) simRecColl[trpart];
    //2) loop on the STA muons matched 
        for (std::vector<std::pair<edm::RefToBase<reco::Track>, double> >::const_iterator IT = simRecAsso.begin();
             IT != simRecAsso.end(); ++IT) {
        //    cout << "inside !! " << endl;
            edm::RefToBase<reco::Track> track = IT->first;
            double quality = IT->second;
            TrajectorySeed theSeed = (*track->seedRef());
            const TrackingRecHit *seghit = &(*(theSeed.recHits().first));
            TransientTrackingRecHit::ConstRecHitPointer ttrh(theLocalMuonRecHitBuilder->build(seghit));
        //    cout << "candidate eta=" << ttrh->globalPosition().eta() << " phi=" << ttrh->globalPosition().phi() << " quality=" << quality << endl;
            if (quality>bestQuality){
                bestQuality=quality;
                theBestQualitySTA = track;
            }
            foundAmatch = true;
        }

    //3) now that we have the STA with the best quality, check its purity
        double purity = -1.;
        if(recSimColl.find(theBestQualitySTA) != recSimColl.end()) {
            std::vector<std::pair<TrackingParticleRef, double> > recSimAsso = recSimColl[theBestQualitySTA];
            for (std::vector<std::pair<TrackingParticleRef, double> >::const_iterator ITS = recSimAsso.begin();
                 ITS != recSimAsso.end(); ++ITS) {
                TrackingParticleRef tp = ITS->first;
                if (tp == trpart) purity = ITS->second;

            }
        }
        /*if (foundAmatch) *theSTAMuon= theBestQualitySTA;
        if (foundAmatch) cout <<(theBestQualitySTA)->pt() << endl;*/
     //   cout << "purity=" << purity <<  endl;
        *theMatchPurity = purity;
    }
     /*       // find the purity from RecoToSim association (set purity = -1 for unmatched recoToSim)
           */
    *trackFound = foundAmatch;
    *theMatchQuality = bestQuality;
    return theBestQualitySTA;
}

TrajectoryStateOnSurface L2seedsAnalyzer::seedTransientState(const TrajectorySeed& tmpSeed){

    PTrajectoryStateOnDet tmpTSOD = tmpSeed.startingState();
    DetId tmpDetId(tmpTSOD.detId());
    const GeomDet* tmpGeomDet = theTrackingGeometry->idToDet(tmpDetId);
    TrajectoryStateOnSurface tmpTSOS = trajectoryStateTransform::transientState(tmpTSOD, &(tmpGeomDet->surface()), &(*theMGField));
    return tmpTSOS;
}


// ------------ method called once each job just after ending the event loop  ------------
void
L2seedsAnalyzer::endJob()
{
    rootFile_->Write();
    rootFile_->Close();
}

void
L2seedsAnalyzer::beginEvent()
{
    T_Muon_Eta = new std::vector<float>;
    T_Muon_Phi = new std::vector<float>;
    T_Muon_Energy = new std::vector<float>;
    T_Muon_Et = new std::vector<float>;
    T_Muon_Pt = new std::vector<float>;
    T_Muon_Px = new std::vector<float>;
    T_Muon_Py = new std::vector<float>;
    T_Muon_Pz = new std::vector<float>;
    T_Muon_Mass = new std::vector<float>;
    
    
    T_Muon_IsGlobalMuon = new std::vector<bool>;
    T_Muon_IsTrackerMuon = new std::vector<bool>;
    T_Muon_IsPFMuon = new std::vector<bool>;
    T_Muon_IsCaloMuon = new std::vector<bool>;
    T_Muon_IsStandAloneMuon = new std::vector<bool>;
    T_Muon_IsMuon = new std::vector<bool>;
    T_Muon_IsGlobalMuon_PromptTight = new std::vector<bool>;
    T_Muon_IsTrackerMuonArbitrated = new std::vector<bool>;
    T_Muon_numberOfChambers = new std::vector<int>;
    T_Muon_numberOfChambersRPC = new std::vector<int>;
    T_Muon_numberOfMatches = new std::vector<int>;
    T_Muon_numberOfMatchedStations = new std::vector<int>;
    T_Muon_charge = new std::vector<int>;

    T_Muon_TMLastStationTight = new std::vector<bool>;
    T_Muon_globalTrackChi2 = new std::vector<float>;
    T_Muon_validMuonHits = new std::vector<int>;
    T_Muon_trkKink = new std::vector<float>;
    T_Muon_trkNbOfTrackerLayers = new std::vector<int>;
    T_Muon_trkNbOfValidTrackeHits = new std::vector<int>;
    T_Muon_trkValidPixelHits = new std::vector<int>;
    T_Muon_trkError = new std::vector<float>;
    T_Muon_dB = new std::vector<float>;
    T_Muon_dzPV = new std::vector<float>;
    
    T_Seed_Muon_nHits = new std::vector<int>;
    T_Seed_Muon_refFirstHit = new std::vector<int>;
    T_Seed_Muon_Eta = new std::vector<float>;
    T_Seed_Muon_Phi = new std::vector<float>;
    T_Seed_Muon_Pt = new std::vector<float>;
    T_Seed_Muon_Pz = new std::vector<float>;
    T_Seed_Muon_Px = new std::vector<float>;
    T_Seed_Muon_Py = new std::vector<float>;
    T_Seed_Muon_PxErr = new std::vector<float>;
    T_Seed_Muon_PyErr = new std::vector<float>;
    T_Seed_Muon_PzErr = new std::vector<float>;
    T_Seed_Muon_PtErr = new std::vector<float>;
    T_Seed_Muon_EtaErr = new std::vector<float>;
    T_Seed_Muon_PhiErr = new std::vector<float>;
    
    T_Hits_Muon_L2Eta = new std::vector<float>;
    T_Hits_Muon_L2Phi = new std::vector<float>;
    T_Hits_Muon_localL2x = new std::vector<float>;
    T_Hits_Muon_localL2y = new std::vector<float>;
    T_Hits_Muon_localL2z = new std::vector<float>;
    T_Hits_Muon_isDT = new std::vector<int>;
    T_Hits_Muon_DTwire = new std::vector<int>;
    T_Hits_Muon_DTlayer = new std::vector<int>;
    T_Hits_Muon_DTsuperlayer = new std::vector<int>;
    T_Hits_Muon_DTWheel = new std::vector<int>;
    T_Hits_Muon_DTStation = new std::vector<int>;
    T_Hits_Muon_DTSector = new std::vector<int>;
    T_Hits_Muon_isCSC = new std::vector<int>;
    T_Hits_Muon_CSClayer = new std::vector<int>;
    T_Hits_Muon_CSCchamber = new std::vector<int>;
    T_Hits_Muon_CSCring = new std::vector<int>;
    T_Hits_Muon_CSCiChamberType = new std::vector<int>;

    
    


    T_Gen_Muon_Px = new std::vector<float>;
    T_Gen_Muon_Py = new std::vector<float>;
    T_Gen_Muon_Pz = new std::vector<float>;
    T_Gen_Muon_Energy = new std::vector<float>;
    T_Gen_Muon_Pt = new std::vector<float>;
    T_Gen_Muon_Eta = new std::vector<float>;
    T_Gen_Muon_Phi = new std::vector<float>;
    T_Gen_Muon_PDGid = new std::vector<int>;
    T_Gen_Muon_status = new std::vector<int>;
    T_Gen_Muon_MotherID = new std::vector<int>;
    T_Gen_Muon_tpPt = new std::vector<float>;
    T_Gen_Muon_tpEta = new std::vector<float>;
    T_Gen_Muon_tpPhi = new std::vector<float>;
    T_Gen_Muon_FoundSTA = new std::vector<int>;
    T_Gen_Muon_StaPt = new std::vector<float>;
    T_Gen_Muon_StaEta = new std::vector<float>;
    T_Gen_Muon_StaPhi = new std::vector<float>;
    T_Gen_Muon_StaSeedEta = new std::vector<float>;
    T_Gen_Muon_StaSeedPhi = new std::vector<float>;
    T_Gen_Muon_StaPurity = new std::vector<float>;
    T_Gen_Muon_StaQuality = new std::vector<float>;
    T_Gen_Muon_FoundL2 = new std::vector<int>;
    T_Gen_Muon_L2Eta = new std::vector<float>;
    T_Gen_Muon_L2Phi = new std::vector<float>;
    T_Gen_Muon_L2Purity = new std::vector<float>;
    T_Gen_Muon_L2Quality = new std::vector<float>;
    T_Gen_Muon_L2crudeMaching = new std::vector<int>;

    
    
}

void
L2seedsAnalyzer::endEvent()
{
    delete T_Muon_Eta;
    delete T_Muon_Phi;
    delete T_Muon_Energy;
    delete T_Muon_Et;
    delete T_Muon_Pt;
    delete T_Muon_Px;
    delete T_Muon_Py;
    delete T_Muon_Pz;
    delete T_Muon_Mass;
    
    delete T_Muon_IsGlobalMuon;
    delete T_Muon_IsTrackerMuon;
    delete T_Muon_IsPFMuon;
    delete T_Muon_IsCaloMuon;
    delete T_Muon_IsStandAloneMuon;
    delete T_Muon_IsMuon;
    delete T_Muon_IsGlobalMuon_PromptTight;
    delete T_Muon_IsTrackerMuonArbitrated;
    delete T_Muon_numberOfChambers;
    delete T_Muon_numberOfChambersRPC;
    delete T_Muon_numberOfMatches;
    delete T_Muon_numberOfMatchedStations;
    delete T_Muon_charge;
    
    
    delete T_Muon_TMLastStationTight;
    delete T_Muon_globalTrackChi2;
    delete T_Muon_validMuonHits;
    delete T_Muon_trkKink;
    delete T_Muon_trkNbOfTrackerLayers;
    delete T_Muon_trkNbOfValidTrackeHits;
    delete T_Muon_trkValidPixelHits;
    delete T_Muon_trkError;
    delete T_Muon_dB;
    delete T_Muon_dzPV;
    
    delete T_Seed_Muon_nHits;
    delete T_Seed_Muon_refFirstHit;
    delete T_Seed_Muon_Eta;
    delete T_Seed_Muon_Phi;
    delete T_Seed_Muon_Pt;
    delete T_Seed_Muon_Pz;
    delete T_Seed_Muon_Px;
    delete T_Seed_Muon_Py;
    delete T_Seed_Muon_PxErr;
    delete T_Seed_Muon_PyErr;
    delete T_Seed_Muon_PzErr;
    delete T_Seed_Muon_PtErr;
    delete T_Seed_Muon_EtaErr;
    delete T_Seed_Muon_PhiErr;
    
    delete T_Hits_Muon_L2Eta;
    delete T_Hits_Muon_L2Phi;
    delete T_Hits_Muon_localL2x;
    delete T_Hits_Muon_localL2y;
    delete T_Hits_Muon_localL2z;
    delete T_Hits_Muon_isDT;
    delete T_Hits_Muon_DTwire;
    delete T_Hits_Muon_DTlayer;
    delete T_Hits_Muon_DTsuperlayer;
    delete T_Hits_Muon_DTWheel;
    delete T_Hits_Muon_DTStation;
    delete T_Hits_Muon_DTSector;
    delete T_Hits_Muon_isCSC;
    delete T_Hits_Muon_CSClayer;
    delete T_Hits_Muon_CSCchamber;
    delete T_Hits_Muon_CSCring;
    delete T_Hits_Muon_CSCiChamberType;

    
    
    delete T_Gen_Muon_Px;
    delete T_Gen_Muon_Py;
    delete T_Gen_Muon_Pz;
    delete T_Gen_Muon_Energy;
    delete T_Gen_Muon_Pt;
    delete T_Gen_Muon_Eta;
    delete T_Gen_Muon_Phi;
    delete T_Gen_Muon_PDGid;
    delete T_Gen_Muon_status;
    delete T_Gen_Muon_MotherID;
    delete T_Gen_Muon_tpPt;
    delete T_Gen_Muon_tpEta;
    delete T_Gen_Muon_tpPhi;
    delete T_Gen_Muon_FoundSTA;
    delete T_Gen_Muon_StaPt;
    delete T_Gen_Muon_StaEta;
    delete T_Gen_Muon_StaPhi;
    delete T_Gen_Muon_StaSeedEta;
    delete T_Gen_Muon_StaSeedPhi;
    delete T_Gen_Muon_StaPurity;
    delete T_Gen_Muon_StaQuality;
    delete T_Gen_Muon_FoundL2;
    delete T_Gen_Muon_L2Eta;
    delete T_Gen_Muon_L2Phi;
    delete T_Gen_Muon_L2Purity;
    delete T_Gen_Muon_L2Quality;
    delete T_Gen_Muon_L2crudeMaching;
    

    
}

// ------------ method called when starting to processes a run  ------------
/*
void 
L2seedsAnalyzer::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a run  ------------
/*
void 
L2seedsAnalyzer::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when starting to processes a luminosity block  ------------
/*
void 
L2seedsAnalyzer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a luminosity block  ------------
/*
void 
L2seedsAnalyzer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
L2seedsAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

float L2seedsAnalyzer::deltaR(float phi1, float phi2, float eta1, float eta2)
{
    float dphi=deltaPhi(phi1,phi2);
    float deta=fabs(eta1-eta2);
    float dr = sqrt(dphi*dphi+ deta*deta);
    return dr;
}

float L2seedsAnalyzer::deltaPhi(float phi1, float phi2)
{
    float dphi;
    if(phi1<0) phi1+=2*TMath::Pi();
    if(phi2<0) phi2+=2*TMath::Pi();
    dphi=fabs(phi1-phi2);
    if(dphi>2*TMath::Pi()) dphi-=2*TMath::Pi();
    if(dphi>TMath::Pi()) dphi=2*TMath::Pi()-dphi;
    return dphi;
}

//define this as a plug-in
DEFINE_FWK_MODULE(L2seedsAnalyzer);
