var source;
var dX, dY, dW, dH, sW, sH;

function getVal_6color(p, i) { // 7.3E 6-color E-Paper (no orange)
    if((p.data[i]==0x00) && (p.data[i+1]==0x00) && (p.data[i+2]==0x00))return 0;
    if((p.data[i]==0xFF) && (p.data[i+1]==0xFF) && (p.data[i+2]==0xFF))return 1;
    if((p.data[i]==0xFF) && (p.data[i+1]==0xFF) && (p.data[i+2]==0x00))return 2;
    if((p.data[i]==0xFF) && (p.data[i+1]==0x00) && (p.data[i+2]==0x00))return 3;
    if((p.data[i]==0x00) && (p.data[i+1]==0x00) && (p.data[i+2]==0xFF))return 5;
    if((p.data[i]==0x00) && (p.data[i+1]==0xFF) && (p.data[i+2]==0x00))return 6;
    return 7;
}

function setVal(p,i,c){
    p.data[i]=curPal[c][0];
    p.data[i+1]=curPal[c][1];
    p.data[i+2]=curPal[c][2];
    p.data[i+3]=255;
}

function addVal(c,r,g,b,k){
    return[c[0]+(r*k)/32,c[1]+(g*k)/32,c[2]+(b*k)/32];
}

function getErr(r,g,b,stdCol){
    r-=stdCol[0];
    g-=stdCol[1];
    b-=stdCol[2];
    return r*r + g*g + b*b;
}

function getNear(r,g,b){
    var ind=0;
    var err=getErr(r,g,b,curPal[0]);
    for (var i=1;i<curPal.length;i++)
    {
        var cur=getErr(r,g,b,curPal[i]);
        if (cur<err){err=cur;ind=i;}
    }
    return ind;
}
