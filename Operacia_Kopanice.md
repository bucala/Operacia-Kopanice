# OPERATION KOPANICE: UE5 REBORN
## Game Design Document (GDD)

## 1. Concept and Technological Transition
Operation Kopanice retains its core identity as a hardcore tactical strategy game emphasizing stealth, asymmetric combat, and specialist synergy[cite: 2]. The game fully transitions from its original 2D isometric Entity Component System architecture[cite: 2] into a fully rendered 3D environment in **Unreal Engine 5** (utilizing Lumen for dynamic global illumination and Nanite for micro-geometry). The AI will determine whether the original grid system (tile-based) will transform into a modern 3D NavMesh or retain a hidden hybrid grid to ensure absolute tactical precision. All visual assets will be modeled exclusively in **Blender**.

## 2. Historical and Narrative Background (SNP 1944)
The campaign takes place in the autumn and winter of 1944, strictly adhering to the historical realities of the Slovak National Uprising, which began on August 29, 1944[cite: 1].
* **Location:** The rugged border terrain of Myjavská pahorkatina and Biele Karpaty[cite: 1, 2].
* **Factions:** The player controls specialists tied to the "Hurban" partisan detachment commanded by Miloš Uher[cite: 1]. The enemy consists of the German Wehrmacht (708th Volksgrenadier Division) and SS units[cite: 1].
* **Objective:** Conduct diversionary actions, disrupt German logistics routes, and tie down enemy forces[cite: 1, 2].

## 3. Characters and Skill Synergy
All characters will be remodeled into high-poly 3D assets. Their uniforms must reflect a historical mix of Czechoslovak military blouses (vz. 30), Soviet gear, and civilian clothing[cite: 1].
* **Leader (Green Beret):** Expert in close-quarters combat and body concealment[cite: 2].
* **Sapper (Żenista):** Specialist in explosives and traps[cite: 2].
* **Spy (Špión):** Ability to disguise as a German officer and issue fake orders[cite: 2].
* **Thief (Zved):** Extreme agility, lockpicking, and vertical traversal[cite: 2].
* **Sniper (Odstreľovač):** Long-range target elimination (utilizing a scoped Mosin-Nagant rifle)[cite: 1, 2].

## 4. Arsenal and Vehicles (Historical Authenticity)
* **Partisan Arsenal:** Reflects limited resources, featuring captured German Kar98k rifles, MP40 submachine guns, Soviet PPŠ-41s, and Panzerfaust anti-tank weapons[cite: 1].
* **German Arsenal:** Standard issue equipment such as the Gewehr 43, StG44, and MG42 heavy machine guns[cite: 1].
* **Vehicles:** Highly detailed, interactive 3D models of the Kübelwagen Typ 82, Opel Blitz 3t cargo trucks, and Panzer IV medium tanks[cite: 1].

## 5. Environment Design and Dynamic Weather
The environment must accurately depict flysch sediments and limestone formations (e.g., Vršatské bradlá), complemented by dense beech forests and traditional wooden log cabins[cite: 1].
* **Atmosphere:** UE5 will drive dynamic weather changes, ranging from thick autumn fog that severely reduces visibility[cite: 2] to harsh winter conditions with freezing temperatures dropping to -15°C[cite: 1].
* **Snow Physics:** Winter will drastically impact gameplay. Deep snow reduces movement speed to 28.5% and leaves trackable footprints that enemy AI can investigate[cite: 2].

## 6. Reworked Campaign and Missions
* **Mission 1: The Iron Throat (Sabotage):** Set in dense autumn fog. The objective is to utilize the Sapper and Leader to steal TNT and destroy a strategic railway viaduct, paralyzing the German logistics corridor from Moravia to Slovakia[cite: 1, 2].
* **Mission 2: Echoes from the Manor (Infiltration):** A rescue mission in a baroque manor occupied by the Einsatzkommando 13 unit[cite: 1, 2]. The Spy uses a German officer disguise to distract guards, while the Thief infiltrates the headquarters via a lightning rod on the north wing to disable the lighting[cite: 2].
* **Mission 3: Showdown in Cetuna (Historical Finale):** A recreation of the real and tragic Battle of Cetuna on February 27, 1945[cite: 1]. The player operates in deep snow. The Sniper's role is precise target elimination from a safe distance, while the Sapper covers the retreat corridor with snow traps[cite: 2]. The mission concludes with the death of commander Miloš Uher (a historical fact), resulting in a dramatic escape for the rest of the unit[cite: 1].

## 7. Cloud Pipeline and GitHub
* Project Repository: `https://github.com/bucala/Operacia-Kopanice.git`
* UE5 project files, Blueprints, and ECS adaptations will be verified by an autonomous assistant that automatically oversees the integrity of 3D assets from Blender and the correct integration of the enemy AI state machine into Unreal behavior trees[cite: 2].