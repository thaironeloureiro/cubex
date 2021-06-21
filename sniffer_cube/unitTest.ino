#line 2 "unitTest.ino"
#if UNIT_TEST ==1

test(getPath)
{
  Serial.println("cheguei aqui");
  char prev_test[MAXNODES];  
  boolean tem_rota_teste;
  prev_test[54]=53;//Dada a posição 54, retorna a posição anterior num caminho calculado.
  prev_test[56]=0; //Dada a posição 56, não há posição anterior para chegar nessa celula - indicando que não há caminho possivel
  
  tem_rota_teste=false;
  tem_rota_teste = getPath(54, prev_test);
  assertEqual(tem_rota_teste, true);

  tem_rota_teste = getPath(56, prev_test);
  assertNotEqual(tem_rota_teste, true);
}
#endif
