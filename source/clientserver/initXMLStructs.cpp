#ifdef HIERARCHICAL_DATA

#  include "initXMLStructs.h"

void initEfit(EFIT* str)
{
    str->device[0] = '\0';
    str->exp_number = 0;
    str->nfluxloops = 0;
    str->nmagprobes = 0;
    str->npfcircuits = 0;
    str->npfpassive = 0;
    str->nplasmacurrent = 0;
    str->ndiamagnetic = 0;
    str->ntoroidalfield = 0;
    str->npfsupplies = 0;
    str->npfcoils = 0;
    str->nlimiter = 0;

    str->fluxloop = NULL;
    str->pfpassive = NULL;
    str->magprobe = NULL;
    str->pfcircuit = NULL;
    str->plasmacurrent = NULL;
    str->diamagnetic = NULL;
    str->toroidalfield = NULL;
    str->pfsupplies = NULL;
    str->pfcoils = NULL;
    str->limiter = NULL;
}

void initInstance(INSTANCE* str)
{
    str->archive[0] = '\0';
    str->file[0] = '\0';
    str->signal[0] = '\0';
    str->owner[0] = '\0';
    str->format[0] = '\0';
    str->status = 0;
    str->seq = 0;
    str->factor = 1.0; // Factor Always applied to Data!
}

void initFluxLoop(FLUXLOOP* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->nco = 0;
    str->r = NULL;
    str->z = NULL;
    str->dphi = NULL;
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initPfPassive(PFPASSIVE* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->nco = 0;
    str->r = NULL;
    str->z = NULL;
    str->dr = NULL;
    str->dz = NULL;
    str->ang1 = NULL;
    str->ang2 = NULL;
    str->res = NULL;
    str->modelnrnz[0] = 0;
    str->modelnrnz[1] = 0;
}

void initPfCoils(PFCOILS* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->aerr = 0.0;
    str->rerr = 0.0;
    str->turns = 0;
    str->nco = 0;
    str->r = NULL;
    str->z = NULL;
    str->dr = NULL;
    str->dz = NULL;
    str->modelnrnz[0] = 0;
    str->modelnrnz[1] = 0;
}

void initMagProbe(MAGPROBE* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->r = 0.0;
    str->z = 0.0;
    str->angle = 0.0;
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initPfSupplies(PFSUPPLIES* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initPfCircuits(PFCIRCUIT* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->nco = 0;
    str->coil = NULL;
    str->supply = 0;
}

void initPlasmaCurrent(PLASMACURRENT* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initDiaMagnetic(DIAMAGNETIC* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initToroidalField(TOROIDALFIELD* str)
{
    str->id[0] = '\0';
    initInstance(&str->instance);
    str->aerr = 0.0;
    str->rerr = 0.0;
}

void initLimiter(LIMITER* str)
{
    str->nco = 0;
    str->factor = 1.0; // Unique as No Instance Child Structure
    str->r = NULL;
    str->z = NULL;
}

// Print Utilities

void printInstance(FILE* fh, INSTANCE str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "archive       : %s\n", str.archive);
    fprintf(fh, "file          : %s\n", str.file);
    fprintf(fh, "signal        : %s\n", str.signal);
    fprintf(fh, "owner         : %s\n", str.owner);
    fprintf(fh, "format        : %s\n", str.format);
    fprintf(fh, "sequence/pass : %d\n", str.seq);
    fprintf(fh, "status        : %d\n", str.status);
    fprintf(fh, "factor        : %f\n", str.factor);
}

void printMagProbe(FILE* fh, MAGPROBE str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Magnetic Probe\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "r          : %f\n", str.r);
    fprintf(fh, "z          : %f\n", str.z);
    fprintf(fh, "angle      : %f\n", str.angle);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
}

void printPfSupplies(FILE* fh, PFSUPPLIES str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "PF Supply\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
}

void printPfCircuits(FILE* fh, PFCIRCUIT str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "PF Circuit\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "supply     : %d\n", str.supply);
    fprintf(fh, "nco        : %d\n", str.nco);
    for (int i = 0; i < str.nco; i++) {
        fprintf(fh, "Coil Connect # %d     : %d\n", i, str.coil[i]);
    }
}

void printFluxLoop(FILE* fh, FLUXLOOP str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Flux Loop\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
    fprintf(fh, "nco        : %d\n", str.nco);
    if (str.nco > 0) {
        for (int i = 0; i < str.nco; i++) {
            fprintf(fh, "r, z, dphi   # %d     : %f   %f   %f\n", i, str.r[i], str.z[i], str.dphi[i]);
        }
    }
}

void printPfCoils(FILE* fh, PFCOILS str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "PF Coil\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
    fprintf(fh, "turns per  : %d\n", str.turns);
    fprintf(fh, "turns per  : %f\n", str.fturns);
    fprintf(fh, "model nr nr: %d  %d\n", str.modelnrnz[0], str.modelnrnz[1]);
    fprintf(fh, "nco        : %d\n", str.nco);
    for (int i = 0; i < str.nco; i++) {
        fprintf(fh, "r, z, dr, dz # %d     : %f   %f   %f   %f\n", i, str.r[i], str.z[i], str.dr[i], str.dz[i]);
    }
}

void printPfPassive(FILE* fh, PFPASSIVE str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "PF Passive\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
    fprintf(fh, "model nr nr: %d  %d\n", str.modelnrnz[0], str.modelnrnz[1]);
    for (int i = 0; i < str.nco; i++) {
        fprintf(fh, "r,z,dr,dz,a1,a2,res   # %d     : %f  %f  %f  %f  %f  %f  %f\n", i, str.r[i], str.z[i], str.dr[i],
                str.dz[i], str.ang1[i], str.ang2[i], str.res[i]);
    }
}

void printPlasmaCurrent(FILE* fh, PLASMACURRENT str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Plasma Current\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
}

void printDiaMagnetic(FILE* fh, DIAMAGNETIC str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Diamagnetic Flux\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
}

void printToroidalField(FILE* fh, TOROIDALFIELD str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Toroidal Field\n");
    fprintf(fh, "id         : %s\n", str.id);
    printInstance(fh, str.instance);
    fprintf(fh, "aerr       : %f\n", str.aerr);
    fprintf(fh, "rerr       : %f\n", str.rerr);
}

void printLimiter(FILE* fh, LIMITER str)
{
    if (&str == NULL) {
        return;
    }
    fprintf(fh, "Limiter\n");
    fprintf(fh, "factor     : %f\n", str.factor);
    fprintf(fh, "nco        : %d\n", str.nco);
    for (int i = 0; i < str.nco; i++) {
        fprintf(fh, "r, z   # %d     : %f    %f\n", i, str.r[i], str.z[i]);
    }
}

void printEFIT(FILE* fh, EFIT str)
{
    fprintf(fh, "EFIT Hierarchical Structure\n");
    fprintf(fh, "Device     : %s\n", str.device);
    fprintf(fh, "Exp. Number: %d\n", str.exp_number);

    if (str.fluxloop != NULL) {
        printFluxLoop(fh, *(str.fluxloop));
    }
    if (str.magprobe != NULL) {
        printMagProbe(fh, *(str.magprobe));
    }
    if (str.pfcircuit != NULL) {
        printPfCircuits(fh, *(str.pfcircuit));
    }
    if (str.pfpassive != NULL) {
        printPfPassive(fh, *(str.pfpassive));
    }
    if (str.plasmacurrent != NULL) {
        printPlasmaCurrent(fh, *(str.plasmacurrent));
    }
    if (str.toroidalfield != NULL) {
        printToroidalField(fh, *(str.toroidalfield));
    }
    if (str.pfsupplies != NULL) {
        printPfSupplies(fh, *(str.pfsupplies));
    }
    if (str.pfcoils != NULL) {
        printPfCoils(fh, *(str.pfcoils));
    }
    if (str.limiter != NULL) {
        printLimiter(fh, *(str.limiter));
    }
    if (str.diamagnetic != NULL) {
        printDiaMagnetic(fh, *(str.diamagnetic));
    }
}

#endif
