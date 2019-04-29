function dim = size(ob)
%function dim = size(ob)
%       "size" method for sense_svd class

AA = ob.A;
sen = ob.VS;
nc = size(sen,2);

%dim = [nc*size(AA.tt,1) size(AA.we,1)];
%dim = [nc*size(AA,1) size(AA,2)];
tmp = size(AA);
dim = [nc*tmp(1) tmp(2)];
