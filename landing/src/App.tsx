import './App.css'
import Gallifreyan from './utils/Gallifreyan'

function App() {
  return (
    <>
      <h1 className='m-10 p-4 bg-red-400 rounded-2xl'>Hello!</h1>      
      <Gallifreyan width={1000} height={1000}
      text={`At the time of William Ward Duffield's death, Herbert Maryon was 38 years old and a teacher of sculpture at the University of Reading in Reading, Berkshire.
        [25] One of his colleagues was Walter Geoffrey Duffield, a professor of physics at Reading and the grandson of William Ward Duffield's brother Walter.
        [26][note 4] Maryon had studied at The Slade, Saint Martin's School of Art, and the Central School of Arts and Crafts,[28][25] where his teachers included Alexander Fisher and William Lethaby.
        [29][30] He subsequently led the Keswick School of Industrial Art from 1900 to 1904, where he designed numerous Arts and Crafts works, and taught metalwork at the Storey Institute.
        [28] Maryon taught at Reading from 1907 to 1927, then at Armstrong College until 1939.[31] He designed a number of other memorials while a teacher, including First World War monuments for East Knoyle,[32] Mortimer,[33] and the University of Reading.
        [34] After the Second World War, Maryon went on a second career as a conservator at the British Museum; his work on the Sutton Hoo ship-burial led to his appointment as an Officer of the Order of the British Empire.[35]`} 
      lineSpread={1.1} thickness={1} sentenceSpacing={1} spiralFactor={0.05} wordSpacing={0.5} />
    </>
  )
}

export default App
